#!/usr/bin/ruby

require 'open3'

# start zyn, grep the lo server port, and connect the port checker to it
Open3.popen3(Dir.pwd + "/../zynaddsubfx -O null --no-gui") do |stdin, stdout, stderr, wait_thr|
  pid = wait_thr[:pid]
  while line=stderr.gets do 
    # print "line: " + line;
    if /^lo server running on (\d+)$/.match(line) then
      # Timeout 500 is currently required because MiddleWare can be too busy to reply when it
      # calls OscilGen::get thousands of times in a row to calculate the whole wave table at once.
      # This should be changed once MiddleWare will calculate the wave table bit by bit and will
      # be more responsive.
      rval = system(Dir.pwd + "/../../rtosc/port-checker --timeout 500 'osc.udp://localhost:" + $1 + "/'")
      Process.kill("KILL", pid)
      exit rval
    end
  end
end
