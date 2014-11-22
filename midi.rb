require "unimidi"

# Prompt the user
#input = UniMIDI::Input.gets
name = 'Faderfox DJ3'

input = UniMIDI::Input.select { |m| m.name == name }.first.open

module XNORMIDI
  SYSEX_BEGIN = 0xF0
  SYSEX_END = 0xF7

  STATUSMASK = 0x80
  CHANMASK = 0x0F

  CC = 0xB0
  NOTEON = 0x90
  NOTEOFF = 0x80
  AFTERTOUCH = 0xA0
  PITCHBEND = 0xE0
  PROGCHANGE = 0xC0
  CHANPRESSURE = 0xD0

  CLOCK = 0xF8
  TICK = 0xF9
  START = 0xFA
  CONTINUE = 0xFB
  STOP = 0xFC
  ACTIVESENSE = 0xFE
  RESET = 0xFF

  TC_QUARTERFRAME = 0xF1
  SONGPOSITION = 0xF2
  SONGSELECT = 0xF3
  TUNEREQUEST = 0xF6

  def is_status?(byte)
    byte & STATUSMASK == STATUSMASK
  end

  def is_realtime?(byte)
    byte >= CLOCK
  end

  def channel(status)
    status & CHANMASK
  end

  def packet_type(status)
    if (status & 0xF0) == 0xF0
      return status
    end
    return status & 0xF0
  end

  def packet_length(status) 
    case (status & 0xF0)
    when CC, NOTEON, NOTEOFF, AFTERTOUCH, PITCHBEND
      return 3
    when PROGCHANGE, CHANPRESSURE, SONGSELECT
      return 2
    when 0xF0
      case status
      when CLOCK, TICK, START, CONTINUE, STOP, ACTIVESENSE, RESET, TUNEREQUEST
        return 1
      when SONGPOSITION
        return 3
      when TC_QUARTERFRAME, SONGSELECT
        return 2
      #when SYSEX_END, SYSEX_BEGIN
      else
        return nil
      end
    end
    return nil
  end
end

include XNORMIDI

loop do
  @funcs = Hash.new
  @funcs[CC] = lambda do |arr|
    puts "cc chan: #{channel(arr[0])} num: #{arr[1]} val: #{arr[2]}"
  end
  @funcs[NOTEON] = lambda do |arr|
    puts "note on chan: #{channel(arr[0])} num: #{arr[1]} vel: #{arr[2]}"
  end
  @funcs[NOTEOFF] = lambda do |arr|
    puts "note off chan: #{channel(arr[0])} num: #{arr[1]} vel: #{arr[2]}"
  end

  #puts(m)
  cur_cntdown = 0
  current = []
  input.gets.each do |m|
    m[:data].each do |d|
      if is_realtime?(d)
        f = @funcs[d]
        f.call(d) if f
      elsif is_status?(d)
        cur_cntdown = packet_length(d) - 1
        current = [d]
      elsif current
        current << d
        cur_cntdown = cur_cntdown - 1
        if cur_cntdown <= 0 and current
          f = @funcs[packet_type(current.first)]
          f.call(current) if f
          current = nil
        end
      end
    end
  end
end

