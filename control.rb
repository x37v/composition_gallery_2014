require 'rubygems'
require 'osc-ruby'

client = OSC::Client.new('localhost', 3333)
client.send(OSC::Message.new( "/greeting" , "hullo!"))
