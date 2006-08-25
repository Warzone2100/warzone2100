#!/usr/bin/env ruby

def enumerate_libraries_and_frameworks(file, skip)
  otool_results = `otool -L #{file}`

  libraries = []
  frameworks = []

  lines = otool_results.split("\n")
  lines.shift

  lines.each do |line|
    path = line
    path.gsub! /^\s*(.*)\s\(.*\)$/, '\1'
    skip_it = false
    skip.each do |skip_path|
      if path.index skip_path
        skip_it = true
      end
    end
    if ! skip_it
      if path =~ /\.framework/
        frameworks.push path
      else
        libraries.push path
      end
    end
  end

  return libraries, frameworks
end

executable = ARGV[0]
appdir = ARGV[1]

skip = ['/usr/lib/libSystem.B.dylib',
	'/usr/lib/libmx.A.dylib',
	'/usr/lib/libicucore.A.dylib',
	'/usr/lib/libauto.dylib',
	'/usr/lib/libobjc.A.dylib',
	'/usr/lib/system/libmathCommon.A.dylib',
	'/usr/lib/libgcc_s.1.dylib',
	'/usr/lib/libz.1.dylib',
	'/usr/lib/libxml2.2.dylib',
	'/usr/lib/libsqlite3.0.dylib',
	'/usr/lib/libcrypto.0.9.7.dylib',
	'/usr/lib/libbsm.dylib',
	'/usr/lib/libcups.2.dylib',
	'/usr/lib/libedit.2.dylib',
	'/usr/lib/libstdc++.6.dylib',
	'/usr/lib/libiconv.2.dylib',
	'/usr/lib/libncurses.5.4.dylib',
	'/System/Library/Frameworks/OpenGL.framework',
	'/System/Library/Frameworks/Cocoa.framework',
	'/System/Library/Frameworks/AudioToolbox.framework',
	'/System/Library/Frameworks/AudioUnit.framework',
	'/System/Library/Frameworks/CoreFoundation.framework',
	'/System/Library/Frameworks/CoreAudio.framework',
	'/System/Library/Frameworks/vecLib.framework',
	'/System/Library/Frameworks/IOKit.framework',
	'/System/Library/Frameworks/AppKit.framework',
	'/System/Library/Frameworks/SystemConfiguration.framework',
	'/System/Library/Frameworks/CoreData.framework',
	'/System/Library/Frameworks/Foundation.framework',
	'/System/Library/Frameworks/CoreServices.framework',
	'/System/Library/Frameworks/Security.framework',
	'/System/Library/Frameworks/DiskArbitration.framework',
	'/System/Library/Frameworks/QuartzCore.framework',
	'/System/Library/Frameworks/ApplicationServices.framework',
	'/System/Library/Frameworks/Accelerate.framework',
	'/System/Library/Frameworks/Carbon.framework',
	'/System/Library/Frameworks/QuickTime.framework']

libraries, frameworks = enumerate_libraries_and_frameworks executable, skip

all_libraries = libraries
all_frameworks = frameworks

found = -1
until found == 0
  found = 0
  tmpframeworks = all_frameworks
  tmpframeworks.each do |search|
    libraries, frameworks = enumerate_libraries_and_frameworks search, skip
    diff = libraries - all_libraries
    found += diff.length
    all_libraries += diff
    diff = frameworks - all_frameworks
    found += diff.length
    all_frameworks += diff
  end
end

found = -1
until found == 0
  found = 0
  tmplibraries = all_libraries
  tmplibraries.each do |search|
    libraries, frameworks = enumerate_libraries_and_frameworks search, skip
    diff = libraries - all_libraries
    found += diff.length
    all_libraries += diff
    diff = frameworks - all_frameworks
    found += diff.length
    all_frameworks += diff
  end
end

system "mkdir -p '#{appdir}/Contents/MacOS'"
if ! all_frameworks.empty?
  system "mkdir -p '#{appdir}/Contents/Frameworks'"
end
if ! all_libraries.empty?
  system "mkdir -p '#{appdir}/Contents/Resources/lib'"
end

system "cp '#{executable}' '#{appdir}/Contents/MacOS/'"
executable_relative = executable.gsub /^.*\/([^\/]*)$/, '\1'

all_frameworks.each do |framework|
  dirname = framework.gsub /^(.*)\.framework\/.*$/, '\1.framework'
  relative = framework.gsub /^.*\/([^\/]*\.framework.*)$/, '\1'
  system "cp -R '#{dirname}' '#{appdir}/Contents/Frameworks/'"
  system "install_name_tool -change '#{framework}' '@executable_path/../Frameworks/#{relative}' '#{appdir}/Contents/MacOS/#{executable_relative}'"
end

all_libraries.each do |library|
  relative = library.gsub /^.*\/([^\/]*)$/, '\1'
  system "cp '#{library}' '#{appdir}/Contents/Resources/lib/'"
  system "install_name_tool -id '#{relative}' '#{appdir}/Contents/Resources/lib/#{relative}'"
  system "install_name_tool -change '#{library}' '@executable_path/../Resources/lib/#{relative}' '#{appdir}/Contents/MacOS/#{executable_relative}'"
end

all_libraries.each do |library1|
  relative1 = library1.gsub /^.*\/([^\/]*)$/, '\1'
  all_libraries.each do |library2|
    relative2 = library2.gsub /^.*\/([^\/]*)$/, '\1'
    system "install_name_tool -change '#{library2}' '@executable_path/../Resources/lib/#{relative2}' '#{appdir}/Contents/Resources/lib/#{relative1}'"
  end
end

