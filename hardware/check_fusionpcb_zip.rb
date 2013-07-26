#!/usr/bin/env ruby
# -*- coding: utf-8 -*-

if ARGV.size != 1 then
  abort "usage: ruby #{$0} path/to/fusionpcb.zip"
end

# Top Layer: pcbname.GTL
# Bottom Layer: pcbname.GBL
# Solder Mask Top: pcbname.GTS
# Solder Mask Bottom: pcbname.GBS
# Silk Top: pcbname.GTO
# Silk Bottom: pcbname.GBO
# Drill Drawing: pcbname.TXT
# Board Outline：pcbname.GML/GKO
spec = {
  "GTL" => "Top Layer",
  "GBL" => "Bottom Layer",
  "GTS" => "Solder Mask Top",
  "GBS" => "Solder Mask Bottom",
  "GTO" => "Silk Top",
  "GBO" => "Silk Bottom",
  "TXT" => "Drill Drawing",
  "GML" => "Board Outline",
}

# qq: very quiet
#  l: list
stdout = `unzip -qql #{ ARGV[0] }`
lines  = stdout.split("\n")
# puts stdout

ok = 1
spec.each_key { |extension|
  if ! lines.find {|line| line.match(/\.#{extension}/)}
    puts "#{spec[extension]} (#{extension}) not found"
    ok = nil
  end
}

if ok
  puts "zip file looks fine."
end
