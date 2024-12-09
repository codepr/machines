#!/usr/bin/env ruby

Token = Struct.new(:type, :value)

class Lexer
  TOKEN_TYPES = [
    [:lbracket, /\[/],
    [:rbracket, /\]/],
    [:unary_instr, /\b[a-zA-Z]+\b/],
    [:integer, /\b[0-9]+\b/],
    [:string, /"([^"\\]*(\\.[^"\\]*)*)"/],
    [:operator, /\*|\/|\+|\-/],
    [:newline, /\n/]
  ]

  def initialize(code)
    @code = code
  end

  def tokenize
    tokens = []
    until @code.empty?
      tokens << next_token
      @code = @code.strip
    end
    tokens << Token.new(:eof, nil)
  end

  private

  def next_token
    TOKEN_TYPES.each do |type, re|
      re = /\A(#{re})/
      if @code =~ re
        value = $1
        @code = @code[value.length..-1]
        return Token.new(type, value)
      end
    end

    raise RuntimeError.new("Unexpected token #{@code.inspect}")
  end
end

class ASTNode
  attr_reader :source_name, :ast

  def initialize(source_name, ast)
    @source_name = source_name
    @ast = ast
  end

  def print(indent = 0)
    puts "#{' ' * indent}ASTNode (#{@source_name})"
    ast.each { |node| node.print(indent + 2) }
  end
end

class EOFNode
  attr_reader :name

  def initialize(name)
    @name = name
  end

  def print(indent = 0)
    puts "#{' ' * indent}EOFNode ()"
  end
end

class UnaryInstrNode
  attr_reader :name

  def initialize(name)
    @name = name
  end

  def print(indent = 0)
    puts "#{' ' * indent}UnaryInstrNode (#{name})"
  end
end

class IntegerNode
  attr_reader :value

  def initialize(value)
    @value = value
  end

  def print(indent = 0)
    puts "#{' ' * indent}IntegerNode (#{value})"
  end
end

class OperatorNode
  attr_reader :operator

  OPERATORS = {
    "+" => "ADD",
    "-" => "SUB",
    "*" => "MUL",
    "/" => "DIV",
  }

  def initialize(operator)
    @operator = operator
  end

  def commutative?
    ["+", "*", "&", "|"].include?(@operator)
  end

  def print(indent = 0)
    puts "#{' ' * indent}OperatorNode (#{operator})"
  end

  def asm_operator
    "#{OPERATORS[@operator]}"
  end
end

class Parser
  def initialize(source_name, tokens)
    @source_name = source_name
    @tokens = tokens
    @position = 0
  end

  def parse
    src = []
    while @position < @tokens.length
      src << parse_sexpr
    end
    ASTNode.new(@source_name, src)
  end

  private

  def current_token
    @tokens[@position]
  end

  def advance
    @position += 1
  end

  def parse_sexpr
    case current_token.type
    when :operator
      parse_operator
    when :integer
      parse_integer
    when :lbracket
      advance
      expr = parse_sexpr
      if current_token && current_token.type == :rbracket
        advance # consume ')'
      end
      expr
    when :rbracket
      advance
      parse_sexpr
    when :unary_instr
      parse_instruction
    when :newline
      advance
      parse_sexpr
    when :eof
      advance
      EOFNode.new("eof")
    else
      raise "Unexpected token: #{current_token.inspect}"
    end
  end

  def parse_operator
    operator = current_token.value
    advance
    OperatorNode.new(operator)
  end

  def parse_integer
    IntegerNode.new(current_token.value.to_i).tap { advance }
  end

  def parse_instruction
    UnaryInstrNode.new(current_token.value).tap { advance }
  end

end

class Compiler

  def initialize
    @pc = 0
  end

  def run(node)
    case node
    when ASTNode
      generate_ast_asm(node)
    when OperatorNode
      generate_operator_asm(node)
    when IntegerNode
      generate_integer_asm(node)
    when UnaryInstrNode
      generate_instruction_asm(node)
    when EOFNode
      generate_eof_asm
    else
      raise "Unknown node type"
    end
  end

  def generate_instruction_asm(node)
    case node.name
    when "dup"
      asm = "    %04d DUP" % @pc
      @pc +=1
      asm
    when "fold"
      asm = "    %04d ADD" % @pc
      @pc +=1
      asm
    when "puts"
      asm = "    %04d PRINT" % @pc
      @pc +=1
      asm
    end
  end

  def generate_ast_asm(node)
    asm = node.ast.map { |node| run(node) }
    asm.unshift(".text\n")
    asm.unshift("# アトム VM target code - #{node.source_name}.atom\n")
    asm.join("\n")
  end

  def generate_eof_asm
    asm = "    %04d HALT" % @pc
    @pc +=1
    asm
  end

  def generate_operator_asm(node)
    asm = "    %04d #{node.asm_operator}" % @pc
    @pc += 1
    asm
  end

  def generate_integer_asm(node)
    asm = "    %04d PUSH_CONST #{node.value}" % @pc
    @pc += 2
    asm
  end
end

def compile(source)
  source_name = (source.split(/\//)[-1]).split(/\./)[0]
  tokens = Lexer.new(File.read(source)).tokenize()
  tree = Parser.new(source_name, tokens).parse
  tree.print
  puts Compiler.new.run(tree)
end

compile("test.sl")
