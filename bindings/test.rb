require 'keyvalcfg'

require 'pp'

node = KeyvalCfg.parse_file(ARGV[0])

pp node
