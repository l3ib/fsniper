require 'keyvalcfg'

cfg = KeyvalCfg.parse('../test/correctness/1.cfg')
puts cfg.find('irrationals').find('root two').comment
puts cfg.find('irrationals').find('root two').value
