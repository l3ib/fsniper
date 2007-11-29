#!/usr/bin/ruby

success = true
valgrind = false

if ARGV.include? '-v'
	valgrind = true
end

Dir.entries('.').sort.each do |entry|
	next if entry =~ /^..?$/
	next unless entry =~ /\.c$/
	
	entry = File.basename(entry, '.c')
	
	# compile
	
	puts "test ##{entry}\n\tcompiling..."
	
	unless system("gcc", "-Wall", "-Werror", "-pedantic", "-g", "-g3", "-ggdb",
                "-o", entry, "-I../src/", "#{entry}.c", "../src/keyvalcfg.c")
		puts "\t\tfailed"
		success = false
		break
	end
	
	puts "\t\tdone"
	
	# execute
	
	puts "\trunning... "
	
	command = valgrind ? ["valgrind", "--leak-check=full"] : []
	command << "#{File.dirname(__FILE__)}/#{entry}"
	command << "#{entry}.cfg"
	
	unless system(*command)
		puts "\t\tfailed"
		success = false
		break
	end
	
	puts "\t\tpassed"
end

if success
	puts "all tests passed."
end
