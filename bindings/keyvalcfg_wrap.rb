module KeyvalCfg
	class ParseError < RuntimeError
		def new(*args)
			super(*args)
		end
	end
	class Node
		attr_accessor :children, :name, :value, :comment
		def initialize
			@children = []
			@name = ''
			@value = ''
			@comment = ''
		end
	end
end
