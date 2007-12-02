#!/usr/bin/ruby


success = true

Dir.entries('.').each do |test|
	next unless File.ftype(test) == 'directory'
	next if test =~ /^..?$/

	# compile
	sources = []

	if test.class == Hash
		name = test[:name]
		if test[:source]
			sources << test[:source]
		end
	else
		name = test
	end

	sources += Dir.glob("#{name}/*.c")
	sources.flatten!

	puts "#{name}:\n\tcompiling..."
	
	command = ["gcc", "-Wall", "-Werror", "-pedantic", "-g", "-g3", "-ggdb",
	           "-o", "test-tmp", "-I../src/", "../src/keyvalcfg.c"]

	command += sources

	unless system(*command)
		puts "\t\tfailed"
		success = false
		break
	end

	break unless success

	puts "\t\tdone"
	
	# execute
	
	puts "\trunning... "
	
	if File.exist? "#{name}/description.rb"
		load "#{name}/description.rb"
		cases = $cases.map do |argv|
			[File.join(name, argv[0]), argv[1..-1]]
		end
	else
		cases = Dir.glob("#{name}/*.cfg").sort
	end

	cases.each do |input|
		command = ["valgrind", "--leak-check=full", "--error-exitcode=2",
		           "--quiet", "--suppressions=suppressions"]
		command << "./test-tmp"
		command << input

		command.flatten!
		
		puts "\t\tcase #{input.class == Array ? input[0] : input}:"
		unless system(*command)
			puts "\t\t\tfailed"
			success = false
			break
		end

		if $? == 2
			puts "\t\t\tfailed (see valgrind)"
			success = false
			break
		end

		puts "\t\t\tpassed"
	end
	break unless success
end

if success
	puts "all tests passed."
end
