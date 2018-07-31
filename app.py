import subprocess

p = subprocess.Popen('./bin/calibrate')
p.wait()
p.kill()
