#!/usr/bin/ruby

TESTS = [
	'correctness',
	'list-escape',
	'multiline-value',
	{
		:name => 'no-value',
		:source => 'common/error.c'
	},
	{
		:name => 'not-closed',
		:source => 'common/error.c'
	},
	{
		:name => 'not-closed-2',
		:source => 'common/error.c'
	}
]

success = true
valgrind = false

if ARGV.include? '-v'
	valgrind = true
end

TESTS.each do |test|	
	# compile

	sources = []

	if test.class == Hash
		name = test[:name]
		if test[:source]
			if test[:source].class == Array
				sources += test[:source]
			else
				sources << test[:source]
			end
		end
	else
		name = test
	end

	sources += Dir.glob("#{name}/*.c")

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
	
	Dir.glob("#{name}/*.cfg").sort.each do |input|
		command = valgrind ? ["valgrind", "--leak-check=full"] : []
		command << "./test-tmp"
		command << input
		
		puts "\t\tcase #{input}:"
		unless system(*command)
			puts "\t\t\tfailed"
			success = false
			break
		end

		break unless success

		puts "\t\t\tpassed"
	end
	break unless success
end

if success
	puts "all tests passed."
end
