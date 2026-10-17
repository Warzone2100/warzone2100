#!/usr/bin/env python3
# Summarizes a per-frame camera trace written by --cameratrace=<file>
# (needs a binary configured with -DWZ_PERF_COUNTERS=ON).
#
#   python3 tests/perf/camtrace.py trace.csv [--plot out.png] [--skip SECONDS]
#
# Typical capture, on screen with a heavy scenario and a reproducible pan:
#
#   build/src/warzone2100 --movementbenchwatch=perf_cyborg_blob \
#       --cameratrace=/tmp/cam.csv --cameraautopan=1,0,6,1
#
# What it reports, and how to read it:
#
# - present intervals: the spacing of pie_ScreenFrameRenderEnd() returns.
#   With vsync on and nothing dropped these cluster tightly around the refresh period.
#   "Long" frames (> 1.5x the median) are missed vsyncs.
# - sample point: where in the frame the scroll code read its clock, relative to the loop start,
#   split by whether a sim tick ran in that loop.
#   If tick frames sample much later, the scroll dt includes tick time.
# - scroll dt vs display interval: the dt the scroll code integrated each frame
#   against the median present interval,
#   next to the dt a post-present clock sample would have given. Works even if the camera never moved.
# - pan judder: while moving, the camera displacement per presented frame
#   compared with scroll speed x frame interval.
#   Error is expressed in "ms of motion", which is independent of camera speed.
#   Reported against both the measured present interval
#   and the median interval (what a fixed-refresh display shows when nothing drops).
# - graphicsTime pacing: how far graphicsTime (which positions units, projectiles and effects)
#   advanced each frame compared with what the display showed, by tick / after-tick / other frames.
# - frame pacing: how often the frame clock was rounded to the refresh period, and its drift.
# - actual display times: if <trace>.present.csv exists (a backend with presentation timing),
#   the true display interval of each frame,
#   and the camera, frame clock and graphicsTime errors against it rather than against the median interval.
# - truncation: fraction of each scroll step thrown away by the conversion to integer.
# - height: how often trackHeight() ran, its dt, how the desired height steps,
#   and how often the camera height stood still for a frame in the middle of a height move.
#
# Uses only the standard library (--plot needs matplotlib).
import argparse
import csv
import math
import statistics
import sys


def num(v):
    if v is None or v == "":
        return None
    try:
        return int(v)
    except ValueError:
        return float(v)


def pct(values, p):
    if not values:
        return float("nan")
    s = sorted(values)
    k = (len(s) - 1) * p / 100.0
    lo, hi = math.floor(k), math.ceil(k)
    if lo == hi:
        return s[int(k)]
    return s[lo] + (s[hi] - s[lo]) * (k - lo)


def describe(name, values, unit="ms", fmt="%8.3f"):
    if not values:
        print("  %-34s (no data)" % name)
        return
    sd = statistics.pstdev(values) if len(values) > 1 else 0.0
    cells = [statistics.fmean(values), sd, pct(values, 50), pct(values, 95), pct(values, 99), max(values)]
    print("  %-34s n=%-6d mean " % (name, len(values)) + fmt % cells[0] + "  sd " + fmt % cells[1]
          + "  p50 " + fmt % cells[2] + "  p95 " + fmt % cells[3] + "  p99 " + fmt % cells[4]
          + "  max " + fmt % cells[5] + " " + unit)


def load(path):
    with open(path, newline="") as f:
        rows = [{k: num(v) for k, v in r.items()} for r in csv.DictReader(f)]
    return rows


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("trace")
    ap.add_argument("--skip", type=float, default=0.0, help="ignore the first N seconds of in-game frames (default 0)")
    ap.add_argument("--plot", help="write a PNG with height and pan-velocity traces (needs matplotlib)")
    args = ap.parse_args()

    rows = load(args.trace)
    # Frames that drew the 3D view and presented through gameLoop.
    frames = [r for r in rows if r["t_draw"] is not None and r["t_present"] is not None]
    if len(frames) < 10:
        print("Too few in-game frames in trace (%d)." % len(frames))
        return 1
    t0 = frames[0]["t_present"]
    frames = [r for r in frames if (r["t_present"] - t0) / 1e9 >= args.skip]
    if len(frames) < 10:
        print("Too few frames after --skip.")
        return 1

    ms = 1e-6
    span = (frames[-1]["t_present"] - frames[0]["t_present"]) / 1e9
    print("Trace: %s" % args.trace)
    print("  %d in-game frames over %.1f s (%.1f fps)" % (len(frames), span, (len(frames) - 1) / span if span > 0 else 0))

    # ---- present intervals -------------------------------------------------------------
    pairs = list(zip(frames, frames[1:]))
    present_dt = [(b["t_present"] - a["t_present"]) * ms for a, b in pairs]
    median_dt = pct(present_dt, 50)
    print("\nPresent intervals")
    describe("all frames", present_dt)
    long_frames = [d for d in present_dt if d > 1.5 * median_dt]
    print("  long frames (> 1.5x median %.2f ms): %d (%.2f%%)" % (median_dt, len(long_frames), 100.0 * len(long_frames) / len(present_dt)))
    tick_dt = [(b["t_present"] - a["t_present"]) * ms for a, b in pairs if (b["ticks"] or 0) > 0]
    radar_dt = [(b["t_present"] - a["t_present"]) * ms for a, b in pairs if b["radar"]]
    plain_dt = [(b["t_present"] - a["t_present"]) * ms for a, b in pairs if not (b["ticks"] or 0) and not b["radar"]]
    describe("frames with sim tick(s)", tick_dt)
    describe("frames with radar redraw", radar_dt)
    describe("frames with neither", plain_dt)
    tick_ns = [r["tick_ns"] * ms for r in frames if (r["ticks"] or 0) > 0]
    describe("sim tick cost per tick frame", tick_ns)
    multi = sum(1 for r in frames if (r["ticks"] or 0) > 1)
    print("  frames running more than one tick: %d" % multi)

    # Long frames: which stage of the frame they spent their time in,
    # and whether they lock to a phase of game time (a once-per-game-second event shows up as one dominant phase).
    long_idx = [i + 1 for i, d in enumerate(present_dt) if d > 1.5 * median_dt]
    if long_idx:
        phases = {}
        for i in long_idx:
            ph = (frames[i]["game_time"] or 0) % 1000
            phases[ph] = phases.get(ph, 0) + 1
        top = sorted(phases.items(), key=lambda kv: -kv[1])[:3]
        print("  long frames by gameTime %% 1000: %s" % ", ".join("%d ms: %d" % kv for kv in top))
    if frames[0].get("t_render_begin") is not None or any(r.get("t_render_begin") is not None for r in frames[:50]):
        stages = [("prev present -> loop start", lambda a, b: b["t_loop"] - a["t_present"]),
                  ("loop start -> render (ticks)", lambda a, b: b["t_render_begin"] - b["t_loop"]),
                  ("audio_Update", lambda a, b: b["t_audio_end"] - b["t_render_begin"]),
                  ("widgets/input -> 3D draw", lambda a, b: b["t_draw"] - b["t_audio_end"]),
                  ("3D scene", lambda a, b: b["t_scene_end"] - b["t_draw"]),
                  ("UI, radar, overlays", lambda a, b: b["t_render_end"] - b["t_scene_end"]),
                  ("submit + present (incl. vsync wait)", lambda a, b: b["t_present"] - b["t_render_end"])]
        long_set = set(long_idx)
        print("  frame stages, median ms (normal frames | long frames):")
        for name, fn in stages:
            normal, slow = [], []
            for i in range(1, len(frames)):
                a, b = frames[i - 1], frames[i]
                try:
                    v = fn(a, b) * ms
                except TypeError:
                    continue
                (slow if i in long_set else normal).append(v)
            if normal:
                print("    %-38s %7.2f | %s" % (name, pct(normal, 50), ("%7.2f" % pct(slow, 50)) if slow else "      -"))

    # ---- sample point ------------------------------------------------------------------
    print("\nScroll sample point (time after loop start)")
    s_tick = [(r["t_scroll"] - r["t_loop"]) * ms for r in frames if r["t_scroll"] is not None and r["t_loop"] is not None and (r["ticks"] or 0) > 0]
    s_plain = [(r["t_scroll"] - r["t_loop"]) * ms for r in frames if r["t_scroll"] is not None and r["t_loop"] is not None and not (r["ticks"] or 0)]
    describe("tick frames", s_tick)
    describe("non-tick frames", s_plain)
    rt_off = [(r["t_realtime"] - r["t_present"]) * ms for r in frames if r["t_realtime"] is not None]
    describe("realTimeUpdate after present", rt_off)
    loop_off = [(b["t_loop"] - a["t_present"]) * ms for a, b in pairs if b["t_loop"] is not None]
    describe("next loop start after present", loop_off)

    # ---- scroll dt vs display interval -------------------------------------------------
    # The dt the scroll code integrated each frame, whether or not the camera moved,
    # compared with the median present interval (what a fixed-refresh display shows when nothing drops).
    # "post-present" is the dt the same frame would have had
    # with the clock sampled right after the previous present, P(n-1) - P(n-2).
    print("\nScroll dt vs display interval (%.3f ms)" % median_dt)
    cur_all, cur_tick, post_all, post_tick = [], [], [], []
    clamped = 0
    for i in range(2, len(frames)):
        r = frames[i]
        if not r["scroll_upd"] or r["scroll_dt"] is None:
            continue
        if r["scroll_dt"] >= 0.25:
            clamped += 1
            continue
        near_tick = (r["ticks"] or 0) > 0 or (frames[i - 1]["ticks"] or 0) > 0
        e_cur = r["scroll_dt"] * 1000.0 - median_dt
        e_post = (frames[i - 1]["t_present"] - frames[i - 2]["t_present"]) * ms - median_dt
        cur_all.append(abs(e_cur))
        post_all.append(abs(e_post))
        if near_tick:
            cur_tick.append(abs(e_cur))
            post_tick.append(abs(e_post))
    describe("current sample, all frames", cur_all)
    describe("current sample, tick/after-tick", cur_tick)
    describe("post-present sample, all frames", post_all)
    describe("post-present sample, tick/after-tick", post_tick)
    if clamped:
        print("  scroll dt hit the 0.5 s clamp (or >= 0.25 s) on %d frame(s): a catch-up jump" % clamped)

    # ---- graphicsTime pacing ------------------------------------------------------------
    # graphicsTime drives the interpolated positions of units, projectiles and effects.
    # Compares how far it advanced each frame with the interval the display showed (refresh multiples),
    # split by whether a sim tick ran in that frame or the one before.
    gt_tick, gt_after, gt_other = [], [], []
    for i in range(1, len(frames)):
        a, b = frames[i - 1], frames[i]
        if a["graphics_time"] is None or b["graphics_time"] is None:
            continue
        step = b["graphics_time"] - a["graphics_time"]
        if step == 0:
            continue  # paused, waiting for players, or quitting
        shown = median_dt * max(1, round(present_dt[i - 1] / median_dt))
        err = step - shown
        if (b["ticks"] or 0) > 0:
            gt_tick.append(err)
        elif (a["ticks"] or 0) > 0:
            gt_after.append(err)
        else:
            gt_other.append(err)
    print("\ngraphicsTime step minus display interval (units/effects pacing; signed ms)")
    describe("frames with sim tick", gt_tick, fmt="%8.2f")
    describe("frame after a tick frame", gt_after, fmt="%8.2f")
    describe("other frames", gt_other, fmt="%8.2f")
    all_gt = gt_tick + gt_after + gt_other
    if all_gt:
        print("  mean |error| %.2f ms; frames off by more than 2 ms: %.1f%%"
              % (statistics.fmean([abs(x) for x in all_gt]), 100.0 * sum(1 for x in all_gt if abs(x) > 2) / len(all_gt)))

    # ---- pan judder --------------------------------------------------------------------
    # Moving frames: speed above a floor at both ends of the pair and pointing the same way,
    # no scroll-limit clamp and no war camera.
    # Expected motion uses the mean of the two speeds (exact for the constant acceleration calcScroll applies),
    # so acceleration and deceleration phases count too.
    def vel(r):
        if r["speed_lr"] is None or r["speed_ud"] is None:
            return None
        return (r["speed_lr"], r["speed_ud"])

    speeds = [math.hypot(*vel(r)) for r in frames if vel(r) is not None]
    floor = max(200.0, 0.05 * max(speeds)) if speeds else 0.0
    err_meas, err_med, err_tick, err_after_tick, err_plain = [], [], [], [], []
    vel_t, vel_v = [], []
    for i, (a, b) in enumerate(pairs):
        va, vb = vel(a), vel(b)
        if va is None or vb is None:
            continue
        sa, sb = math.hypot(*va), math.hypot(*vb)
        if sa < floor or sb < floor or va[0] * vb[0] + va[1] * vb[1] <= 0:
            continue
        if a["warcam"] or b["warcam"] or a["limit_hit"] or b["limit_hit"]:
            continue
        v = 0.5 * (sa + sb)
        d = math.hypot(b["cam_x"] - a["cam_x"], b["cam_z"] - a["cam_z"])
        dt = (b["t_present"] - a["t_present"]) * ms
        motion_ms = d / v * 1000.0                  # displacement expressed as ms of motion
        e_meas = motion_ms - dt
        e_med = motion_ms - median_dt * max(1, round(dt / median_dt))
        err_meas.append(e_meas)
        err_med.append(e_med)
        prev_tick = i > 0 and (pairs[i - 1][1]["ticks"] or 0) > 0
        if (b["ticks"] or 0) > 0:
            err_tick.append(abs(e_med))
        elif prev_tick:
            err_after_tick.append(abs(e_med))
        else:
            err_plain.append(abs(e_med))
        vel_t.append((b["t_present"] - t0) / 1e9)
        vel_v.append(d / dt * 1000.0 if dt > 0 else float("nan"))

    print("\nPan judder while moving (error in ms of motion per presented frame, speed floor %.0f u/s)" % floor)
    if err_meas:
        describe("vs measured present interval", [abs(e) for e in err_meas])
        describe("vs median interval (display view)", [abs(e) for e in err_med])
        describe("  ... frames with sim tick", err_tick)
        describe("  ... frame after a tick frame", err_after_tick)
        describe("  ... other frames", err_plain)
        rms = math.sqrt(statistics.fmean([e * e for e in err_med]))
        print("  RMS (display view): %.3f ms of motion, i.e. %.1f%% of a %.2f ms frame" % (rms, 100.0 * rms / median_dt, median_dt))
    else:
        print("  no moving frames (use --cameraautopan, or hold a scroll key)")

    # ---- frame pacing and actual display times ------------------------------------------
    paced = [r for r in frames if r.get("dt_ms") is not None]
    if paced:
        print("\nFrame pacing (frameTimeDeltaSeconds)")
        snapped = sum(1 for r in paced if r["pace_snapped"])
        from_pres = sum(1 for r in paced if r["pace_from_presentation"])
        periods = [r["pace_period_ms"] for r in paced if r["pace_period_ms"]]
        print("  snapped to the refresh period on %d of %d frames (%.1f%%); period %.4f ms%s"
              % (snapped, len(paced), 100.0 * snapped / len(paced), periods[-1] if periods else 0.0,
                 ", from presentation times" if from_pres > len(paced) // 2 else ""))
        describe("|paced - measured| per frame", [abs(r["dt_ms"] - r["dt_measured_ms"]) for r in paced])
        describe("|drift|", [abs(r["pace_drift_ms"]) for r in paced if r["pace_drift_ms"] is not None])
        # Predicted time is taken back from the motion delta a fraction of a period at a time while rounding,
        # and at once otherwise, while a stall is left out in whole periods
        left_out = []
        for r in paced:
            if r.get("dt_motion_ms") is None or not r["pace_period_ms"]:
                continue
            gap = r["dt_ms"] - r["dt_motion_ms"]
            if not r["pace_snapped"]:
                gap -= r.get("dt_predicted_ms") or 0.0
            if gap > (0.75 * r["pace_period_ms"] if r["pace_snapped"] else 1e-3):
                left_out.append(gap)
        if left_out:
            print("  stall time left out of camera motion on %d frames: %.1f ms in total, largest %.1f ms"
                  % (len(left_out), sum(left_out), max(left_out)))
        predicted = [r["dt_predicted_ms"] for r in paced if r.get("dt_predicted_ms")]
        if predicted:
            print("  frames predicted to be late: %d, %.1f ms of camera motion added in advance"
                  % (len(predicted), sum(predicted)))

    present_path = args.trace + ".present.csv"
    display = {}
    try:
        with open(present_path, newline="") as f:
            for r in csv.DictReader(f):
                display[int(r["present_id"])] = int(r["actual_present_ns"]) * 1e-6
    except OSError:
        pass
    if display:
        # Each row's present_id is the frame it presented.
        # Its display interval is the time from the previous present reaching the screen to this one doing so.
        print("\nActual display times (%d frames reported by the presentation engine)" % len(display))
        shown = []
        for a, b in pairs:
            pa, pb = a.get("present_id"), b.get("present_id")
            if pa is None or pb is None or pb != pa + 1 or pa not in display or pb not in display:
                continue
            shown.append((a, b, display[pb] - display[pa]))
        # Without vsync, frames can share a display time or arrive out of order (ex. tearing),
        # so the reported times say little about when each frame became visible
        unordered = sum(1 for _, _, x in shown if x <= 0.0)
        if shown and unordered > 0.01 * len(shown):
            print("  %d of %d display intervals are zero or negative (display times not tied to vblanks, ex. vsync off):"
                  " not compared" % (unordered, len(shown)))
            shown = []
        if shown:
            ivals = [x for _, _, x in shown]
            period = pct(ivals, 50)
            describe("display interval", ivals)
            late = sum(1 for x in ivals if x > 1.5 * period)
            print("  frames shown more than one refresh after the previous: %d (%.2f%%)" % (late, 100.0 * late / len(ivals)))
            # The frame clock is sampled after each present,
            # so the delta recorded on frame a is the one frame b animates by,
            # and b's display interval is what it should have been.
            describe("|frame time delta - display interval|", [abs(a["dt_ms"] - x) for a, b, x in shown if a.get("dt_ms") is not None])
            # Visible discontinuities: a frame held on screen for longer than it animated (a missed vblank),
            # or one that animated by more than it was shown for (a catch-up jump).
            signed = [a["dt_ms"] - x for a, b, x in shown if a.get("dt_ms") is not None]
            held = sum(1 for e in signed if e < -0.5 * period)
            jumped = sum(1 for e in signed if e > 0.5 * period)
            print("  frames animated by half a refresh or more less than shown (holds): %d, more than shown (jumps): %d"
                  % (held, jumped))
            describe("|measured interval - display interval|", [abs(a["dt_measured_ms"] - x) for a, b, x in shown if a.get("dt_measured_ms") is not None])
            # Camera motion per displayed frame against the true display interval
            errs = []
            for a, b, x in shown:
                if a["speed_lr"] is None or b["speed_lr"] is None:
                    continue
                va, vb = (a["speed_lr"], a["speed_ud"]), (b["speed_lr"], b["speed_ud"])
                sa, sb = math.hypot(*va), math.hypot(*vb)
                if sa < 255 or sb < 255 or va[0] * vb[0] + va[1] * vb[1] <= 0 or a["limit_hit"] or b["limit_hit"] or a["warcam"] or b["warcam"]:
                    continue
                moved = math.hypot(b["cam_x"] - a["cam_x"], b["cam_z"] - a["cam_z"])
                errs.append(moved / (0.5 * (sa + sb)) * 1000.0 - x)
            if errs:
                describe("camera motion - display interval", [abs(e) for e in errs])
                rms = math.sqrt(statistics.fmean([e * e for e in errs]))
                print("  RMS against actual display times: %.3f ms of motion, %.1f%% of a %.2f ms refresh" % (rms, 100.0 * rms / period, period))
            gt = []
            for a, b, x in shown:
                if a["graphics_time"] is not None and b["graphics_time"] is not None and b["graphics_time"] != a["graphics_time"]:
                    gt.append(abs((b["graphics_time"] - a["graphics_time"]) - x))
            describe("|graphicsTime step - display interval|", gt)

    # ---- truncation --------------------------------------------------------------------
    # x_exact/z_exact are the exact world-space steps, and x_dif/z_dif what was applied to the integer position.
    # Per update they always differ by under a unit.
    # What matters is whether the difference is thrown away (net loss grows with every update)
    # or carried to the next frame (net loss stays under a unit).
    # Frames clamped by the scroll limits are left out.
    per_update, stepmag = [], []
    net = total = 0.0
    for r in frames:
        if r["scroll_upd"] and r["x_exact"] is not None and not r["limit_hit"]:
            ex = math.hypot(r["x_exact"], r["z_exact"])
            if ex > 0:
                per_update.append(math.hypot(r["x_exact"] - r["x_dif"], r["z_exact"] - r["z_dif"]))
                stepmag.append(ex)
                # shortfall along the direction of travel: positive when motion was lost
                net += ((r["x_exact"] - r["x_dif"]) * r["x_exact"] + (r["z_exact"] - r["z_dif"]) * r["z_exact"]) / ex
                total += ex
    print("\nScroll truncation")
    if per_update:
        describe("exact minus applied, per update", per_update, unit="u")
        describe("exact step per update", stepmag, unit="u")
        print("  net motion lost along the direction of travel: %.1f u of %.0f u (%.3f%%)" % (net, total, 100.0 * net / total))
        upd = sum(1 for r in frames if r["scroll_upd"])
        print("  scroll updated on %d of %d frames that sampled it" % (upd, sum(1 for r in frames if r["t_scroll"] is not None)))
    else:
        print("  no scroll movement in trace")

    # ---- height ------------------------------------------------------------------------
    print("\nHeight follow")
    tracked = [r for r in frames if r["desired_h"] is not None]
    if tracked and len(set(r["desired_h"] for r in tracked)) == 1:
        print("  desired height never changed (flat terrain under the camera): no height data;"
              " pan over hills for this section")
    if tracked:
        ran = sum(1 for r in tracked if r["h_upd"] == 1)
        rest = sum(1 for r in tracked if r["h_upd"] == 2)
        gated = len(tracked) - ran - rest
        print("  frames: %d, solver ran %d (%.1f%%), resting %d, skipped by 16 ms gate %d (%.1f%%)"
              % (len(tracked), ran, 100.0 * ran / len(tracked), rest, gated, 100.0 * gated / len(tracked)))
        describe("solver dt", [r["h_dt_ms"] for r in tracked if r["h_upd"] == 1], fmt="%8.1f")
        steps = [b["desired_h"] - a["desired_h"] for a, b in zip(tracked, tracked[1:]) if b["desired_h"] != a["desired_h"]]
        # How steeply the target changes per unit of camera movement.
        # A continuous target stays within the terrain's own slopes (about 1-2),
        # while a stepped one jumps tens of units over a few units of travel.
        slopes = []
        for a, b in zip(tracked, tracked[1:]):
            moved = math.hypot(b["cam_x"] - a["cam_x"], b["cam_z"] - a["cam_z"])
            if moved >= 1 and not b["warcam"]:
                slopes.append(abs(b["desired_h"] - a["desired_h"]) / moved)
        jumps = sum(1 for x in slopes if x > 3.0)
        avg_steps = [b["avg_h"] - a["avg_h"] for a, b in zip(tracked, tracked[1:]) if b["avg_h"] != a["avg_h"]]
        print("  desired-height changes: %d, largest %d u; change per unit of camera travel p99 %.2f, max %.2f; jumps (> 3 u/u): %d"
              % (len(steps), max([abs(x) for x in steps], default=0), pct(slopes, 99) if slopes else 0.0, max(slopes, default=0.0), jumps))
        print("  average-terrain changes: %d, mean |step| %.1f u" % (len(avg_steps), statistics.fmean([abs(s) for s in avg_steps]) if avg_steps else 0.0))
        # Stalls: frames drawn while the camera is still away from its target but whose height didn't change.
        # NOTE: Traces from builds that ran trackHeight() after drawing
        # record the update feeding frame n+1 on frame n, so a skipped update on either frame counts.
        moving = stalls = gated_stalls = 0
        for a, b in zip(tracked, tracked[1:]):
            if abs(a["desired_h"] - a["cam_y"]) <= 2:
                continue
            moving += 1
            if b["cam_y"] == a["cam_y"]:
                stalls += 1
                if a["h_upd"] == 0 or b["h_upd"] == 0:
                    gated_stalls += 1
        if moving:
            print("  frames with no vertical motion while away from target: %d of %d (%.1f%%), %d of them because the 16 ms gate skipped the update"
                  % (stalls, moving, 100.0 * stalls / moving, gated_stalls))
        stale = max(100.0, 2.5 * median_dt)
        snaps = [r for r in tracked if r["h_upd"] == 1 and r["h_dt_ms"] is not None and r["h_dt_ms"] > stale]
        if snaps:
            print("  solver steps with dt > %.0f ms (stale timestamp, camera snaps to target): %d, largest %.0f ms" % (stale, len(snaps), max(r["h_dt_ms"] for r in snaps)))
        overshoot = sum(1 for a, b in zip(tracked, tracked[1:]) if a["h_sep_after"] is not None and b["h_sep_after"] is not None
                        and a["desired_h"] == b["desired_h"] and a["h_sep_after"] * b["h_sep_after"] < 0)
        print("  overshoot sign flips at constant target: %d" % overshoot)
    else:
        print("  no height-tracking frames (war camera active throughout?)")

    if args.plot:
        try:
            import matplotlib
            matplotlib.use("Agg")
            import matplotlib.pyplot as plt
        except ImportError:
            print("\n--plot needs matplotlib", file=sys.stderr)
            return 0
        t = [(r["t_present"] - t0) / 1e9 for r in tracked]
        fig, (ax1, ax2, ax3) = plt.subplots(3, 1, figsize=(12, 9), sharex=True)
        ax1.plot(t, [r["desired_h"] for r in tracked], drawstyle="steps-post", label="desired height")
        ax1.plot(t, [r["cam_y"] for r in tracked], label="camera height")
        ax1.plot(t, [r["avg_h"] + 500 for r in tracked], linewidth=0.8, alpha=0.6, label="avg terrain + pivot")
        ax1.set_ylabel("world units")
        ax1.legend(loc="upper right")
        ax2.plot(vel_t, vel_v, linewidth=0.7)
        ax2.set_ylabel("pan speed per\npresented frame (u/s)")
        tp = [(b["t_present"] - t0) / 1e9 for a, b in pairs]
        ax3.plot(tp, present_dt, linewidth=0.7, label="present interval")
        ax3.scatter([(r["t_present"] - t0) / 1e9 for r in frames if (r["ticks"] or 0) > 0],
                    [r["tick_ns"] * ms for r in frames if (r["ticks"] or 0) > 0], s=4, color="tab:red", label="sim tick cost")
        ax3.set_ylabel("ms")
        ax3.set_xlabel("seconds")
        ax3.legend(loc="upper right")
        fig.tight_layout()
        fig.savefig(args.plot, dpi=110)
        print("\nWrote %s" % args.plot)
    return 0


if __name__ == "__main__":
    sys.exit(main())
