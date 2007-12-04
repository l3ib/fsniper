require 'keyvalcfg'

node = KeyvalCfg.parse_file(ARGV[0])
node.write
