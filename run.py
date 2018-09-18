#!/usr/bin/python3

import subprocess
import argparse
import sys
import os

BENCH=os.path.dirname(os.path.abspath(__file__)) + "/posix_spawn_bench"

def get_parser():
  parser = argparse.ArgumentParser(description=__doc__)
  parser.add_argument('benchmark',
                      help='What to benchmark',
                      choices=('posix_spawn', 'fork_exec', 'vfork_exec'))
  return parser

def run_bench(benchmark):
  x_axis = list(range(50, 500, 50))
  y_axis = []

  for rss in x_axis:
    rss = str(rss * 1024 * 1024)
    proc = subprocess.Popen([BENCH, '-b', benchmark, '-r', rss],
                            stdout=subprocess.PIPE)
    try:
      outs, errs = proc.communicate()
      outs = outs.decode("utf-8")
      y_axis += [ float(outs.split()[4].split('=')[1]) ]
    except subprocess.TimeoutExpired:
      proc.kill()
      continue
    if proc.wait() != 0:
      print('error: rss=%d' % rss)
      sys.exit(1)

  return (x_axis, y_axis)

def write_data(x_axis, y_axis, fname):
  with open(fname, "w") as output:
    for x, y in zip(x_axis, y_axis):
      output.write("%lf %lf\n" % (x, y))
  output.close()

def main(argv):
  parser = get_parser()
  opts = parser.parse_args(argv)
  x_axis, y_axis = run_bench(opts.benchmark)
  write_data(x_axis, y_axis, opts.benchmark + ".dat")
