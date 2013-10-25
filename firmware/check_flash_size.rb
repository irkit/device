#!/usr/bin/env ruby

if ARGV.size != 2 then
  abort "usage: ruby #{$0} path/to/firmware.hex max-flash-size\nif flash size is more than max (NG): exit 1\nif less (OK): exit 0"
end

def flash_size (filename)
  last_data_row = IO.readlines(filename).reverse.find { |row|
    row.match("^:10(....)00")
  }
  if ! last_data_row
    return 0
  end

  address = Regexp.last_match[1]
  return address.hex
end

def main (filename, max_flash_size)
  size = flash_size(filename)
  if ! size
    abort "couldn't detect flash size"
  end

  if size > max_flash_size then
    STDERR.puts "#{filename} is #{size} bytes, bigger than max #{max_flash_size} bytes !!"
    exit 1;
  else
    puts "#{filename} is #{size} bytes"
  end
end

main( ARGV[0], ARGV[1].to_i )

exit 0;
