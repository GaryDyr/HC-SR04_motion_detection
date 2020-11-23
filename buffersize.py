"""
  Given a set of static measurements, recommend at least 2000, module calculates
  the theorectical "jump" required to detect motion (assumed constant velocity)
  using an ultrasonic sensor, where jump is the difference in microseconds
  between buffer elements.
  Data is not saved. Appears on the console.

  REQUIREMENTS: 
  REQUIRES A CSV FILE. ONE PING ECHO VALUE PER ROW, FIRST LINE HEADER ONLY. 
  FILENAME MUST BEGIN WITH PREFIX "static". Module will use only the most
  recently created static file, no matter what comes after the prefix,
  except for filetype.

  CHECK AND CHANGE AS NEEDED THE SETTINGS BELOW

  make sure installed packages include....pip install numpy, scipy
"""

import os
import os.path
import platform
import re
import sys
import numpy as np
from scipy import stats
from datetime import datetime, timedelta
import string
import csv
import time

#-------CHANGE SETTINGS HERE--------------
#GET PATH TO CSV FILES
path = "F:/python_files/csvfiles" 
#Name of first 6 characters of csv file name; 
#length can be changed in def walk_files, but right now
#searches for file with first 6 lettes as:
csv_prefix = 'static' 
input_path = 'F:/python_files/csvfiles'
#input_path = sys.argv[1] #uncomment to run from PS
print('input path is: ', input_path)
os.chdir('F:/python_files/csvfiles')

#SET THE OBJECT VELOCITY AS THE TARGET
obj_vel = 1  #obj vel in cm/s; 
#SET THE PING INTERVAL in milliseconds
ping_interval = 33
#Constants
#vel. of sound cm/us
snd_vel = 0.03446 #at 22 C and 40% Rh, atmospheric pressure 
#-------------------------------------------


currentDay = datetime.now().day
currentMonth = datetime.now().month
currentYear = datetime.now().year
nowtime = currentYear+currentMonth

cwd = os.getcwd()
dir_path = os.path.dirname(os.path.realpath(__file__))
print('python module directory is: ', dir_path)
print('Current working dir. is: ', cwd)

def walk_files(directory_path):
  #gets the latest file data.
  print('searching directory path for csv file: ', directory_path)
  #get first ctime file come across in directory
  for root, _, filenames in os.walk(directory_path):
    for filename in filenames:
      #print("filename:",filename[:6])
      if filename[:6] == csv_prefix:
        old_date = os.path.getctime(filename)
        latest_file = filename
        break  
    # Walk through files in directory_path, including subdirectories
    #os.walk yields a 3-tuple (dirpath, dirnames, filenames).
    # "_" in tuple is a sort of don't care variable marker.
    for root, _, filenames in os.walk(directory_path):
      for filename in filenames:
        if filename[:6] == csv_prefix:
          file_date = os.path.getctime(filename)
          if file_date > old_date:
            latest_file = filename
            old_date = file_date
      if latest_file[:6] == csv_prefix:
        return latest_file
      else:
        return 'None' 

#Start the process
#get the csvfile with the static echoes using the np method.
csvfile = ""
csvfile = walk_files(path)
if not csvfile == 'None':
  print('file to open: ', csvfile)
  print('Found file; Please wait, importing may take some time...')
  #genfromtxt seems a bit slow, but works
  static_array = np.genfromtxt(csvfile, skip_header=1)
else:
  print('Error: The csv file with prefix name ' + csv_prefix + ' was not found. Aborting.')
  print('Looked in directory: ', input_path)
  quit()
#convert to numpy array and transpose
if static_array.size == 0:
  print('Something wrong. No static echoes in static_array; size is 0')
  exit()
#First get the stdev of the buffer from data read in as csv_list

def get_stddev():
  #takes the global defined static_array and calculates the slope for 
  #each buffer size array
  #create the buffer array to hold the slope values
  sub_factor = 0.90
  static_subrng = int(sub_factor*static_array.size)
  slopes_array = np.zeros(static_subrng) #-2 because start at index 2'
  #We will not store all the buffer limits for each buffer size, but 
  #just the  current instance.
  #obj vel in cm/s; ping_interval ms  
  usec_difference = 2*obj_vel*ping_interval*0.001/snd_vel
  print('static_array size is:', static_array.size)
  print('will use ',static_subrng )
  for buffnum in range(2, static_subrng):
    if buffnum > static_array.size - static_subrng:
      print('Buffer length exceeds size, ', sub_factor, ' of static array...aborting')
      exit()
    x = np.arange(buffnum)
    for i in range(static_subrng):
      buff = static_array[i: i+ buffnum]
      #get slope of each buff
      slope, intercept, r_value, p_value, std_err = stats.linregress(x, buff)
      slopes_array[i] = slope
    #get the 2 sigma slope of all the static slopes 
    slope2sigma = np.std(slopes_array)*2
    #print(slope2sigma, usec_difference, buffnum) #for testing
    if slope2sigma <= usec_difference:
      print('Buffer size to match velocity of ', obj_vel, " cm/s is ", buffnum)
      print('Ping Interval (ms)', ping_interval)
      print('Slope limit target from static array was: ', usec_difference)
      print('Slope-limit of this buffer is: ', slope2sigma)
      break
  
get_stddev()
print('Finished')  