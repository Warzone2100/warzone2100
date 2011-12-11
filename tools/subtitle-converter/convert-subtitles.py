#!/usr/bin/env python2
# Convert subtitle timings from frames to seconds.
# Original author: Beliar (http://developer.wz2100.net/ticket/748)
# Changed to use subsecond precision.

def parseLine(line):
    tokens = line.split('\t')
    try:
        tokens.remove("")
    except ValueError:
        pass
    xpos = int(tokens[0])
    ypos = int(tokens[1])
    framestart = float(tokens[2])
    frameend = float(tokens[3])
    text = tokens[4]
    return (xpos, ypos, framestart, frameend, text)

def convertFile(old_filename, new_filename, fps = 25):
    text_file = open(old_filename)
    new_file = open(new_filename, "w")
    try:
        for line in text_file:
            if(line.strip()):
                try:
                    valueList = list(parseLine(line))
                    valueList[2] /= fps
                    valueList[3] /= fps
                    new_file.write("{0}\t{1}\t\t{2}\t{3}\t{4}".format(*valueList))
                except ValueError:
                    new_file.write(line)
                    continue
            else:
                new_file.write("\n")

    finally:
        new_file.close()
        text_file.close()

def convertFiles(old_dir, new_dir, video_dir = "", fps = 25):
    """Converts the the subtitle files in old_dir and subdirectories
    from using fps to using the actual time
    @param old_dir The directory where the to be converted subtititle files are
    @param new_dir Where the changed files should go
    @param video_dir Directory where the video files, for which the 
    subtitles in old_dir where written, are. This is optional but recommended.
    @param fps This is the default fps used to calculate the time. 
    Will be overwritten by the actual fps of the video if video_dir is set    
    """
    import os
    use_video_fps = os.path.exists(video_dir)
    if use_video_fps:
        from theora import Theora
    if use_video_fps:
        print "Using fps values from the actual video files"
    else:
        print "Using the default fps: " + str(fps)
    for dirpath, dirnames, filenames in os.walk (old_dir):
        os.mkdir (os.path.join (new_dir, dirpath[1+len (old_dir):]))
        for filename in filenames:
            old_file = (os.path.join(dirpath, filename))
            new_file = (os.path.join(new_dir, dirpath[1+len (old_dir):], filename))
            if use_video_fps:
                video_file = os.path.join(video_dir, dirpath[1+len (old_dir):], os.path.splitext(filename)[0] + ".ogg")
                video = Theora(video_file)
                fps = int(video.fps_ratio[0] / video.fps_ratio[1])
            print old_file + "->" + new_file
            convertFile(old_file, new_file, fps)

if __name__ == "__main__":
    import argparse
    parser = argparse.ArgumentParser(description='Convert wz2100 subtitle files')
    parser.add_argument('old_dir', metavar='old_dir', type=str,
                   help='Directory where the original subtitle files are')
    parser.add_argument('new_dir', metavar='new_dir', type=str,
                   help='Directory where the new subtitle files go to')
    parser.add_argument('-video_dir', '--v' , metavar='video_dir', type=str, dest="video_dir",
                   help='Directory where the original video files are. Used to get the fps from them.')
    parser.add_argument('-fps', '--f', metavar='fps', type=int, dest="fps",
                   help='FPS to use. Defaults to 25')
    got_args = parser.parse_args().__dict__
    args = {}
    for arg in got_args:
        if got_args[arg]:
            args[arg] = got_args[arg]
    convertFiles(**args)
