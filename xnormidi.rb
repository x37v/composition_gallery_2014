class XNORMIDI
  def initialize
    @funcs = Hash.new
  end

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

  def XNORMIDI.is_status?(byte)
    byte & STATUSMASK == STATUSMASK
  end

  def XNORMIDI.is_realtime?(byte)
    byte >= CLOCK
  end

  def XNORMIDI.channel(status)
    status & CHANMASK
  end

  def XNORMIDI.packet_type(status)
    if (status & 0xF0) == 0xF0
      return status
    end
    return status & 0xF0
  end

  def XNORMIDI.packet_length(status) 
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

  def process(midi_byte_array)
    midi_byte_array.each do |d|
      if XNORMIDI::is_realtime?(d)
        f = @funcs[d]
        f.call(d) if f
      elsif XNORMIDI::is_status?(d)
        len = XNORMIDI::packet_length(d)
        next unless len #possibly nil
        @cur_cntdown = len - 1
        @current = [d]
      elsif @current and @cur_cntdown
        @current << d
        @cur_cntdown = @cur_cntdown - 1
        if @cur_cntdown <= 0
          f = @funcs[XNORMIDI::packet_type(@current.first)]
          f.call(@current) if f
          @current = nil
        end
      end
    end
  end

  def process_unimidi(unimidi)
    unimidi.gets.each do |m|
      process(m[:data])
    end
  end
  
  def register(midi_type, &block)
    @funcs[midi_type] = block
  end
end
