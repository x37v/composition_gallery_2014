require "unimidi"
require_relative 'xnormidi'

MIDI_NAME = 'Faderfox DJ3'

xmidi = XNORMIDI.new

xmidi.register(XNORMIDI::CC) do |arr|
  puts "cc chan: #{XNORMIDI::channel(arr[0])} num: #{arr[1]} val: #{arr[2]}"
end
xmidi.register(XNORMIDI::NOTEON) do |arr|
  puts "note on chan: #{XNORMIDI::channel(arr[0])} num: #{arr[1]} vel: #{arr[2]}"
end
xmidi.register(XNORMIDI::NOTEOFF) do |arr|
  puts "note off chan: #{XNORMIDI::channel(arr[0])} num: #{arr[1]} vel: #{arr[2]}"
end

unimidi = UniMIDI::Input.select { |m| m.name == MIDI_NAME }.first.open
loop do
  xmidi.process_unimidi(unimidi)
end

