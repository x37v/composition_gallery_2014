require 'rubygems'
require 'osc-ruby'

@c = OSC::Client.new('localhost', 9001)
def osend(addr, val)
  if val
    @c.send(OSC::Message.new(addr, val))
  else
    @c.send(OSC::Message.new(addr))
  end
end

osend("/bvca" , 0.0)
osend("/nvca" , 0.0)
osend("/tvca" , 0.0)

osend("/npan" , 0.5)

osend("/bfreq" , 32)
osend("/formanttime" , 0)

vowels = [ 'ee', 'ih', 'eh', 'ae', 'a', 'oo', 'uh', 'o', 'ah' ].collect { |v| "/vowel_" + v }

osend("/nvca" , 1.0)

loop do
  osend(vowels[rand(vowels.size())], nil)
  sleep 0.001
end
