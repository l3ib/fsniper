require 'keyvalcfg_wrap'

module KeyvalCfg

	def KeyvalCfg.parse(*args)
		parse_file(*args)
	end

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

		def write(path = nil)
			_write(path)
		end

		def find(name)
			@children.find do |child|
				child.name == name
			end
		end

		def find_all(name)
			@children.find_all do |child|
				child.name == name
			end
		end
	end
end
