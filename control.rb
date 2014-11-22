require 'rubygems'
require 'osc-ruby'
require "unimidi"
require_relative 'xnormidi'

MIDI_NAME = 'Faderfox DJ3'

xmidi = XNORMIDI.new

@loop_time = 0.001
@omutex = Mutex.new
@c = OSC::Client.new('localhost', 9001)
def osend(addr, val)
  @omutex.synchronize do
    if val
      @c.send(OSC::Message.new(addr, val))
    else
      @c.send(OSC::Message.new(addr))
    end
  end
end

osend("/bvca" , 0.0)
osend("/nvca" , 0.0)
osend("/tvca" , 0.0)

osend("/npan" , 0.5)

osend("/bfreq" , 32)
osend("/formanttime" , 0)

vowels = [ 'ee', 'ih', 'eh', 'ae', 'a', 'oo', 'uh', 'o', 'ah' ].collect { |v| "/vowel_" + v }

@cc_funcs = {
  112 => lambda do |val| #left volume
    osend('/nvca', (val.to_f / 127))
  end,
  0 => lambda do |val| #left filter
    @loop_time = val.to_f / 127
  end
}

xmidi.register(XNORMIDI::CC) do |arr|
  num = arr[1].to_i
  val = arr[2].to_i
  puts "cc chan: #{XNORMIDI::channel(arr[0])} num: #{num} val: #{val}"
  f = @cc_funcs[num]
  f.call(val) if f
end
xmidi.register(XNORMIDI::NOTEON) do |arr|
  puts "note on chan: #{XNORMIDI::channel(arr[0])} num: #{arr[1]} vel: #{arr[2]}"
end
xmidi.register(XNORMIDI::NOTEOFF) do |arr|
  puts "note off chan: #{XNORMIDI::channel(arr[0])} num: #{arr[1]} vel: #{arr[2]}"
end

unimidi = UniMIDI::Input.select { |m| m.name == MIDI_NAME }.first.open

Thread.new do
  loop do
    xmidi.process_unimidi(unimidi)
  end
end

t = Time.now
loop do
  n = Time.now
  if @loop_time == 0
    t = n
  elsif t + @loop_time <= n
    osend(vowels[rand(vowels.size())], nil)
    t = n
  end
end

