#!/usr/bin/ruby

TESTS = [
	{
		:name => 'correctness',
		:source => 'correctness.c',
		:input => 'correctness.cfg'
	},
	{
		:name => 'no value error',
		:source => ['no-value.c', 'error.c'],
		:input => 'no-value.cfg'
	},
	{
		:name => 'not closed error',
		:source => ['not-closed.c', 'error.c'],
		:input => 'not-closed.cfg'
	},
	{
		:name => 'not closed error (#2)',
		:source => ['not-closed-2.c', 'error.c'],
		:input => 'not-closed-2.cfg'
	},
	{
		:name => 'multi-line value',
		:source => 'multiline-value.c',
		:input => 'multiline-value.cfg'
	}
]

success = true
valgrind = false

if ARGV.include? '-v'
	valgrind = true
end

TESTS.each do |test|	
	# compile
	
	puts "#{test[:name]}:\n\tcompiling..."
	
	command = ["gcc", "-Wall", "-Werror", "-pedantic", "-g", "-g3", "-ggdb",
	           "-o", "test-tmp", "-I../src/", "../src/keyvalcfg.c"]

	if test[:source].class == Array
		test[:source].each do |file|
			command << file
		end
	else
		command << test[:source]
	end

	unless system(*command)
		puts "\t\tfailed"
		success = false
		break
	end
	
	puts "\t\tdone"
	
	# execute
	
	puts "\trunning... "
	
	command = valgrind ? ["valgrind", "--leak-check=full"] : []
	command << "./test-tmp"
	command << test[:input]
	
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
