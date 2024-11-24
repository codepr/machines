#!/usr/bin/env ruby

# Crude high level compiler for basic cases of a simple LISP that compiles down to
# a rather unoptimized assembly targetting the stack machine Atom.


# Lexical analysis (tokenization)
# Scan throught the source code and genrate tokens without applying any
# semantic analysis, conceptually, just merely breaking a long string
# into atoms.

Token = Struct.new(:type, :value)

class Lexer
  TOKEN_TYPES = [
    [:lparen, /\(/],
    [:rparen, /\)/],
    [:def, /\bdef\b/],
    [:puts, /\bputs\b/],
    [:identifier, /\b[a-zA-Z]+\b/],
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

# Parsing,semantic analysis of the tokens from the lexer, the goal is to build
# an abstract syntax tree which will provide a semantic context to the grammar
# of the language
#
# Currently, keeping things simple, only a couple of nodes are defined:
#
# - ASTNode        contains the entire source, it's the root of the tree
# - DefNode        contains the nodes defining the body of a function
# - IdentifierNode anything that is not a paren, integer or operator, such
#                  as a function name
# - OperatorNode   defines a mathematical operation, being a LISP, it can
#                  contain multiple operands, in essence it's actually a
#                  function applied to a list of elements
# - IntegerNode    represents an integer value
#
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

class DefNode
  attr_reader :name, :body

  def initialize(name, body)
    @name = name
    @body = body
  end

  def print(indent = 0)
    puts "#{' ' * indent}DefNode (#{name})"
    body.each { |node| node.print(indent + 2) }
  end
end

class IdentifierNode
  attr_reader :name

  def initialize(name)
    @name = name
  end

  def print(indent = 0)
    puts "#{' ' * indent}IdentifierNode (#{name})"
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

class CallNode
  attr_reader :name

  def initialize(name)
    @name = name
  end

  def print(indent = 0)
    puts "#{' ' * indent}CallNode (#{name})"
  end
end

class OperatorNode
  attr_reader :operator, :operands

  OPERATORS = {
    "+" => "ADD",
    "-" => "SUB",
    "*" => "MUL",
    "/" => "DIV",
  }

  def initialize(operator, operands)
    @operator = operator
    @operands = operands
  end

  def commutative?
    ["+", "*", "&", "|"].include?(@operator)
  end

  def print(indent = 0)
    puts "#{' ' * indent}OperatorNode (#{operator})"
    operands.each { |operand| operand.print(indent + 2) }
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
    when :def
      parse_def
    when :operator
      parse_operator
    when :identifier
      parse_identifier
    when :integer
      parse_integer
    when :lparen
      advance
      expr = parse_sexpr
      if current_token && current_token.type == :rparen
        advance # consume ')'
      end
      expr
    when :rparen
      advance
      parse_sexpr
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

  def parse_def
    advance # consume 'def'
    identifier = parse_identifier
    body = []
    while current_token && current_token.type != :rparen && current_token.type != :eof
      body << parse_sexpr
    end
    advance unless current_token.nil? # consume ')'
    DefNode.new(identifier.name, body)
  end

  def parse_operator
    operator = current_token.value
    advance
    operands = []
    while current_token.type != :rparen && current_token.type != :eof
      operands << parse_sexpr
    end
    OperatorNode.new(operator, operands)
  end

  def parse_identifier
    IdentifierNode.new(current_token.value).tap { advance }
  end

  def parse_integer
    IntegerNode.new(current_token.value.to_i).tap { advance }
  end
end

# The compiler itself, scan through the AST starting from the tree and
# attempt to generate an assembly code correctly interpretable by the
# Atom VM.
class Compiler

  def initialize
    @pc = 0
  end

  def run(node)
    case node
    when ASTNode
      generate_ast_asm(node)
    when DefNode
      generate_def_asm(node)
    when OperatorNode
      generate_operator_asm(node)
    when IntegerNode
      generate_integer_asm(node)
    when IdentifierNode
      generate_identifier_asm(node)
    when EOFNode
      generate_eof_asm
    else
      raise "Unknown node type"
    end
  end

  def generate_ast_asm(node)
    asm = node.ast.map { |node| run(node) }
    asm.unshift("# アトム VM target code - #{node.source_name}.atom\n")
    asm.join("\n")
  end

  def generate_eof_asm
    asm = "    %04d HALT" % @pc
    @pc +=1
    asm
  end

  def generate_def_asm(node)
    asm = node.body.map { |stmt| run(stmt) }
    asm.unshift("#{node.name}:")
    asm.join("\n")
  end

  def generate_operator_asm(node)
    asm = node.operands.map { |operand| run(operand) }
    # Order of the operands matters when they're non-commutative, e.g. 5 - 10 != 10 - 5
    # also S-exp are really a cumulative application of the operator to a possibly infinite
    # number of operands and each operand can be a nested S-exp.
    if node.commutative?
      (node.operands.length - 1).times do | _|
        asm << "    %04d #{node.asm_operator}" % @pc
        @pc += 1
      end
    else
      # Dumb trick, when multiple operands for non-commutative operations such as subtraction or
      # division, just sum all of them but last and apply the actual operator at the end
      (node.operands.length - 2).times do |_|
        asm << "    %04d ADD" % @pc
        @pc += 1
      end

      asm << "    %04d #{node.asm_operator}" % @pc
      @pc += 1

    end
    asm
  end

  def generate_integer_asm(node)
    asm = "    %04d PUSH_IMM #{node.value}" % @pc
    @pc += 2
    asm
  end

  def generate_identifier_asm(node)
    asm = "    %04d CALL #{node.name}" % @pc
    @pc += 2
    asm
  end
end

def compile(source)
  source_name = (source.split(/\//)[-1]).split(/\./)[0]
  tokens = Lexer.new(File.read(source)).tokenize()
  tree = Parser.new(source_name, tokens).parse
  puts Compiler.new.run(tree)
end

compile("lisp/basicmath.lisp")
