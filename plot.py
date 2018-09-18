#!/usr/bin/python3

import subprocess
import argparse
import sys
import os

import matplotlib.pyplot as plt
import numpy as np

def read_data(fname):
  x_axis = []
  y_axis = []
  with open(fname, 'r') as fin:
    for line in fin.readlines():
      fields = line.split()
      x_axis.append(float(fields[0]))
      y_axis.append(float(fields[1]))
  return (x_axis, y_axis)

def plot_data(x1, y1, title1, x2, y2, title2, fname):
  n_groups = len(x1)
  index = np.arange(n_groups)
  bar_width = 0.35
  opacity = 0.7

  fig, ax = plt.subplots()
  ax.bar(index,             y1, bar_width, alpha=opacity,
         color='b', label=title1)
  ax.bar(index + bar_width, y2, bar_width, alpha=opacity,
         color='r', label=title2)

  ax.set_xlabel('Process RSS size')
  ax.set_ylabel('Time (s)')
  ax.set_xticks(index + bar_width / 2)
  ax.set_xticklabels(x1)
  ax.legend()

  #fig.tight_layout()
  plt.savefig(fname)

def main(argv):
  x_spawn, y_spawn = read_data('posix_spawn.dat')
  x_fork, y_fork   = read_data('fork_exec.dat')
  x_vfork, y_vfork = read_data('vfork_exec.dat')

  plot_data(x_fork,  y_fork,  'fork + exec',
            x_spawn, y_spawn, 'posix spawn',
            'fork_vs_spawn')
  plot_data(x_vfork, y_vfork, 'vfork + exec',
            x_spawn, y_spawn, 'posix spawn',
            'vfork_vs_spawn')

if __name__ == '__main__':
  main(sys.argv[1:])
