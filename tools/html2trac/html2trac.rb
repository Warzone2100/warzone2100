#!/usr/bin/ruby
#	This file is part of html2trac.
#	Copyright (C) 2008-2009 Dennis Schridde
#
#	html2trac is free software: you can redistribute it and/or modify
#	it under the terms of the GNU General Public License as published by
#	the Free Software Foundation, either version 3 of the License, or
#	(at your option) any later version.
#
#	html2trac is distributed in the hope that it will be useful,
#	but WITHOUT ANY WARRANTY; without even the implied warranty of
#	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#	GNU General Public License for more details.
#
#	You should have received a copy of the GNU General Public License
#	along with html2trac.  If not, see <http://www.gnu.org/licenses/>.


require 'rexml/document'


module REXML
	# Trac Wiki formatter
	# Transforms XHTML 1.1 into Trac Wiki syntax
	#
	# This worked for Warzone 2100's ScriptingManual and likely has to be adapted to suit more generic needs.
	# Do no forget to look at write_element()'s "skip" variable when adjusting it to your documents.
	class Formatters::Trac < Formatters::Default

		def initialize()
			super()
			@listtype = []
			@listdepth = 0
			@idmap = {}
		end

		def write( node, output )
			case node

			when Document
				if node.xml_decl.encoding != "UTF-8" && !output.kind_of?(Output)
					output = Output.new( output, node.xml_decl.encoding )
				end
				write_document( node, output )

			when Element
				write_element( node, output )

			when Declaration, ElementDecl, NotationDecl, ExternalEntity, Entity,
					Attribute, AttlistDecl
				# Ignore

			when Instruction
				write_instruction( node, output )

			when DocType, XMLDecl
				# Ignore

			when Comment
				write_comment( node, output )

			when CData
				write_cdata( node, output )

			when Text
				write_text( node, output )

			else
				raise Exception.new("XML FORMATTING ERROR")

			end
		end

		def write_cdata(node, output)
			fail
		end

		def write_comment(node, output)
			output << "\n{{{\n"
			output << "#!\n"
			output << node.to_s
			output << "\n}}}\n"
		end

		def build_string(node)
			result = ""
			case node
			when Text: result << node.to_s
			when Element: node.children.each { |child| result << build_string(child) }
			end
			result
		end

		def write_document( node, output )
			node.elements.each("//*[@id]") { |id| @idmap[id.attributes["id"]] = build_string(id).delete("(){}[]/+ ") }
			node.children.each { |child| write( child, output ) }
		end

		def write_element(node, output)
			begin_tag = ""
			end_tag = ""
			skip = nil # Set this for nodes you want to skip in your file

			case node.name
				when "html":
				when "head": skip = true
				when "body":
				when "div": skip = node.elements["p/a[@href='top']"]
				when "span":
				when "pre": begin_tag = "\n{{{\n#!c\n" ; end_tag = "\n}}}\n"
				when "table": begin_tag = "\n"
				when "tr": begin_tag = "|" ; end_tag = "|\n"
				when "td": begin_tag = "|" ; end_tag = "|"
				when "th": begin_tag = "|'''" ; end_tag = "'''|"
				when "p": begin_tag = "\n" ; end_tag = "\n"
				when "h1": begin_tag = "\n= " ; end_tag = " =\n"
				when "h2": begin_tag = "\n== " ; end_tag = " ==\n"
				when "h3": begin_tag = "\n=== " ; end_tag = " ===\n"
				when "h4": begin_tag = "\n==== " ; end_tag = " ====\n"
				when "h5": begin_tag = "\n===== " ; end_tag = " =====\n"
				when "b": begin_tag = "'''" ; end_tag = "'''"
				when "i": begin_tag = "''" ; end_tag = "''"
				when "u": begin_tag = "__" ; end_tag = "__"
				when "br": begin_tag = "[[BR]]\n"
				when "hr": begin_tag = "\n----\n"
				when "dl": begin_tag = "\n"
				when "dt": begin_tag = " " ; end_tag = "::\n"
				when "dd": begin_tag = "   " ; end_tag = "\n" if node.next_element
				when "ol":
					if node.elements["ancestor::table"]: # Trac does not support lists/linebreaks inside tables!
						begin_tag = "[[BR]]"
					else
						begin_tag = "\n"
					end
					@listtype.push "ol"
					@listdepth += 1
				when "ul":
					if node.elements["ancestor::table"]: # Trac does not support lists/linebreaks inside tables!
						begin_tag = "[[BR]]"
					else
						begin_tag = "\n"
					end
					@listtype.push "ul"
					@listdepth += 1
				when "li":
					case @listtype.last
						when "ol":
							if (@listdepth % 2) == 0: begin_tag = "  " * @listdepth + "1. "
							else begin_tag = "  " * @listdepth + "a. "
							end
						when "ul": begin_tag = "  " * @listdepth + "* "
					end
					if node.elements["ancestor::table"]: # Trac does not support lists/linebreaks inside tables!
						end_tag = "[[BR]]"
					elsif node.next_element:
						end_tag = "\n"
					end
				when "a":
					href = node.attributes["href"]
					if href == "#top": skip = true end
					if href[0,1] == "#":
						id = href[1..-1]
						begin_tag = "[#" + @idmap[id] + " "
					else
						begin_tag = "[" + href + " "
					end
					end_tag = "]"
				when "acronym": begin_tag = "''" ; end_tag = "''" # Use italic style
				when "code": begin_tag = "`" ; end_tag = "`"
				else raise Exception.new("XHTML FORMATTING ERROR: Unexpected tag: " + node.name)
			end

			if node.children.empty?
				output << begin_tag
			else
				output << begin_tag
				node.children.each { |child|
					write( child, output )
				}
				output << end_tag
			end unless skip

			case node.name
				when "ol": @listtype.pop ; @listdepth -= 1
				when "ul": @listtype.pop ; @listdepth -= 1
			end
		end

		def write_instruction(node, output)
			fail
		end

		def write_text( node, output )
			text = node.to_s
			if ["table", "tr", "dl", "ol", "ul"].include?(node.parent.name):
				text.gsub!(/[[:space:]]+/, '')
			elsif node.parent.name != "pre"
				text.gsub!(/[[:space:]]+/, ' ')
			end
			output << Text::unnormalize(text)
		end
	end
end


xml = REXML::Document.new( STDIN )

REXML::Formatters::Trac.new.write(xml, STDOUT)
