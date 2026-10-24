// SPDX-License-Identifier: GPL-2.0-or-later

/*
	This file is part of Warzone 2100.
	Copyright (C) 2026  Warzone 2100 Project (https://github.com/Warzone2100)

	Warzone 2100 is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	Warzone 2100 is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Warzone 2100; if not, write to the Free Software
	Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
*/

// Standalone unit tests for wz::MainThreadExecQueue (lib/framework/mainthreadexec.h).
// No framework or backend dependencies.
// Exits nonzero on failure.

#include "lib/framework/mainthreadexec.h"

#include <atomic>
#include <cstdio>
#include <stdexcept>
#include <thread>
#include <vector>

static int failures = 0;
static int checks = 0;

#define CHECK(cond, ...) \
	do { \
		++checks; \
		if (!(cond)) { \
			++failures; \
			std::fprintf(stderr, "FAIL %s:%d: %s: ", __FILE__, __LINE__, #cond); \
			std::fprintf(stderr, __VA_ARGS__); \
			std::fprintf(stderr, "\n"); \
		} \
	} while (0)

using Queue = wz::MainThreadExecQueue;

// Counts live instances, to check that every exec is deleted exactly once.
static std::atomic<int> liveExecs(0);

class TestExec : public WZ_MAINTHREADEXEC
{
public:
	explicit TestExec(std::function<void()> fn) : m_fn(std::move(fn)) { ++liveExecs; }
	~TestExec() override { --liveExecs; }
	void doExecOnMainThread() override { m_fn(); }
private:
	std::function<void()> m_fn;
};

static size_t post(Queue& q, std::function<void()> fn)
{
	Queue::ExecPtr exec(new TestExec(std::move(fn)));
	return q.post(exec);
}

static void testFifoAndDeletion()
{
	Queue q;
	std::vector<int> order;
	for (int i = 0; i < 5; ++i)
	{
		CHECK(post(q, [&order, i] { order.push_back(i); }) == static_cast<size_t>(i + 1), "post returns the queued count");
	}
	CHECK(q.pendingCount() == 5, "five pending");
	CHECK(q.process() == 5, "ran five");
	CHECK((order == std::vector<int>{0, 1, 2, 3, 4}), "FIFO order");
	CHECK(liveExecs == 0, "all execs deleted (%d live)", liveExecs.load());
	CHECK(q.pendingCount() == 0, "queue empty");
	CHECK(q.process() == 0, "nothing more to run");
}

static void testDeletedRightAfterRunning()
{
	Queue q;
	int liveDuringSecond = -1;
	post(q, [] {});
	post(q, [&liveDuringSecond] { liveDuringSecond = liveExecs.load(); });
	q.process();
	CHECK(liveDuringSecond == 1, "the first exec was deleted before the second ran (%d live)", liveDuringSecond);
}

static void testPostFromInsideExecRunsNextCall()
{
	Queue q;
	std::vector<int> order;
	post(q, [&] {
		order.push_back(1);
		post(q, [&order] { order.push_back(3); });
	});
	post(q, [&order] { order.push_back(2); });
	CHECK(q.process() == 2, "only the execs queued before the call run");
	CHECK((order == std::vector<int>{1, 2}), "nested post deferred");
	CHECK(q.pendingCount() == 1, "nested post is pending");
	CHECK(q.process() == 1, "nested post runs next call");
	CHECK((order == std::vector<int>{1, 2, 3}), "order kept");
	CHECK(liveExecs == 0, "all execs deleted");
}

static void testNestedProcess()
{
	Queue q;
	std::vector<int> order;
	post(q, [&] {
		order.push_back(1);
		post(q, [&order] { order.push_back(3); });
		q.process(); // a nested drain (ex. from a modal loop) must not clobber the outer batch
	});
	post(q, [&order] { order.push_back(2); });
	q.process();
	CHECK((order == std::vector<int>{1, 3, 2}), "nested drain runs its own batch, outer batch continues");
	CHECK(q.pendingCount() == 0, "nothing left");
	CHECK(liveExecs == 0, "all execs deleted");
}

static void testExceptionRequeuesTheRest()
{
	Queue q;
	std::vector<int> order;
	post(q, [&order] { order.push_back(1); });
	post(q, [] { throw std::runtime_error("boom"); });
	post(q, [&order] { order.push_back(3); });
	post(q, [&order] { order.push_back(4); });
	bool threw = false;
	try
	{
		q.process();
	}
	catch (const std::runtime_error&)
	{
		threw = true;
	}
	CHECK(threw, "exception propagates");
	CHECK((order == std::vector<int>{1}), "stopped at the throwing exec");
	CHECK(liveExecs == 2, "throwing exec deleted, two remain (%d live)", liveExecs.load());

	post(q, [&order] { order.push_back(5); }); // posted after the throw: must stay behind the requeued ones
	CHECK(q.pendingCount() == 3, "requeued two, plus the new one");
	q.process();
	CHECK((order == std::vector<int>{1, 3, 4, 5}), "requeued execs keep FIFO order ahead of later posts");
	CHECK(liveExecs == 0, "all execs deleted");
}

static void testShutdown()
{
	Queue q;
	bool ran = false;
	post(q, [&ran] { ran = true; });
	post(q, [&ran] { ran = true; });
	std::vector<Queue::ExecPtr> abandoned = q.shutdown();
	CHECK(abandoned.size() == 2, "shutdown hands back pending execs");
	CHECK(q.pendingCount() == 0, "queue emptied");

	Queue::ExecPtr late(new TestExec([&ran] { ran = true; }));
	CHECK(q.post(late) == 0, "post refused after shutdown");
	CHECK(late != nullptr, "refused exec stays with the caller");
	CHECK(q.process() == 0, "nothing runs after shutdown");
	CHECK(!ran, "abandoned execs never ran");

	late.reset();
	abandoned.clear();
	CHECK(liveExecs == 0, "all execs deleted");
}

static void testCapacityIsReusedAndBounded()
{
	Queue q;
	int count = 0;
	// A burst well past the retained limit: the spare must be released afterwards...
	for (size_t i = 0; i < Queue::kMaxRetainedCapacity * 2; ++i)
	{
		post(q, [&count] { ++count; });
	}
	q.process();
	CHECK(count == static_cast<int>(Queue::kMaxRetainedCapacity * 2), "burst ran");
	// ...and steady-state use afterwards still works through both buffers.
	for (int round = 0; round < 10; ++round)
	{
		for (int i = 0; i < 100; ++i)
		{
			post(q, [&count] { ++count; });
		}
		q.process();
	}
	CHECK(count == static_cast<int>(Queue::kMaxRetainedCapacity * 2 + 1000), "steady state ran");
	CHECK(liveExecs == 0, "all execs deleted");
}

static void testConcurrentProducers()
{
	Queue q;
	constexpr int kProducers = 4;
	constexpr int kPerProducer = 20000;
	std::vector<int> lastSeen(kProducers, -1);
	bool inOrder = true;
	int ran = 0;

	std::atomic<bool> go(false);
	std::vector<std::thread> producers;
	for (int p = 0; p < kProducers; ++p)
	{
		producers.emplace_back([&, p] {
			while (!go.load()) { std::this_thread::yield(); }
			for (int i = 0; i < kPerProducer; ++i)
			{
				// Runs on the "main" thread below, so these captures are only touched there.
				post(q, [&lastSeen, &inOrder, &ran, p, i] {
					if (lastSeen[p] + 1 != i)
					{
						inOrder = false;
					}
					lastSeen[p] = i;
					++ran;
				});
			}
		});
	}
	go = true;
	while (ran < kProducers * kPerProducer)
	{
		q.process();
	}
	for (auto& t : producers)
	{
		t.join();
	}
	q.process();
	CHECK(ran == kProducers * kPerProducer, "every post ran exactly once (%d)", ran);
	CHECK(inOrder, "each producer's execs ran in the order it posted them");
	CHECK(liveExecs == 0, "all execs deleted");
}

int main()
{
	testFifoAndDeletion();
	testDeletedRightAfterRunning();
	testPostFromInsideExecRunsNextCall();
	testNestedProcess();
	testExceptionRequeuesTheRest();
	testShutdown();
	testCapacityIsReusedAndBounded();
	testConcurrentProducers();

	if (failures != 0)
	{
		std::fprintf(stderr, "%d of %d checks failed\n", failures, checks);
		return 1;
	}
	std::printf("All %d checks passed\n", checks);
	return 0;
}
