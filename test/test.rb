#!/usr/bin/ruby

success = true

Dir.entries('.').sort.each do |entry|
	next if entry =~ /^..?$/
	next unless entry =~ /\.c$/
	
	entry = File.basename(entry, '.c')
	
	# compile
	
	puts "test ##{entry}\n\tcompiling..."
	
	unless system("gcc", "-Wall", "-Werror", "-pedantic", "-o", entry, "-I../src/",
	       "#{entry}.c", "../src/keyvalcfg.c")
		puts "\t\tfailed"
		success = false
		break
	end
	
	puts "\t\tdone"
	
	# execute
	
	puts "\trunning... "
	
	unless system("#{File.dirname(__FILE__)}/#{entry}", "#{entry}.cfg")
		puts "\t\tfailed"
		success = false
		break
	end
	
	puts "\t\tpassed"
end

if success
	puts "all tests passed."
end
