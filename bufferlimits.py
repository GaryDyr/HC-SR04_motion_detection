"""
  Given a set of static measurements, the more the better, recommend at least 2000
  Module calculates the theorectical "jump" required to detect motion (assumed constant
  velocity) using an ultrasonic sensor,where jump is the difference in us between buffer 
  elements. The first non zero value in a buffer (column) indicates the decrease in
  microseconds that must be observed by that element number to declare valid motion. 
  The values are calculated based on the slope_limit. Slope limit is calculated as
  the 2 sigma value of all slopes of the desired buffer size as sub sets of the 
  static array read in to this module. For instance, a buffer size of 10 operating on 
  a static streaming array of 2000 echoes, would produce 1990 buffers of size 10, from 
  which the stdev and the  2 sigma are found. Knowing he slope_limit, the microsecond l
  imits or sensitivity of each buffer can be calculated. 
  In addition, the minimum velocity to meet that sensitivity is also calculated, based
  on the ping interval. 

 make sure install packages ....pip install numpy scipy 
"""

import os
import os.path
import platform
import re
import sys
import pandas as pd
import numpy as np
import matplotlib as mpl
import matplotlib.pyplot as plt
from scipy.optimize import minimize
from scipy.optimize import root
from scipy import stats 
from datetime import datetime, timedelta
import string
import csv

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

#SET THE BUFFER SIZE FOR THIS RUN
buffsize = 10
#SET THE OUT FILE TYPE CSV = 'csv'; NOT IMPLEMENTED EXCEL  = "xl' 
#NOTE: Excel file not implemented...consider...XlsxWriter or openpyxl
out_file_type = 'csv'
#SET THE PING INTERVAL in milliseconds
ping_interval = 33
#SET THE OUTPUT FILENAME; THE BUFFER SIZE WILL BE APPENDED TO NAME
#Set output filename
outfile = 'limits_'
#Constants
#vel. of sound cm/us
snd_vel = 0.03446 #at 22 C and 40% Rh, atmosphereic pressure 
#Set the default x array
x = np.arange(buffsize)
vel = np.zeros(buffsize-1)

data_dict = {}
temp_dict = {}
temp_dict["slope"] = 0.10 # dummy value
data_dict["ping_interval"] = ping_interval
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
#get the csvfile with the static echoes.
csvfile = ""
csvfile = walk_files(path)
if not csvfile == 'None':
  print('Found file;  importing...')
  print('file to open: ', csvfile)
  csv_list = []
  with open(csvfile) as cfile:
    csv_f = csv.reader(cfile)
    headers = next(csv_f, None)  # skip the header, but keep around
    csv_list = []
    for row in csv_f:
      #print('echo us:', row)
      csv_list.append(row)
else:
  print('Error: The csv file with prefix name ' + csv_prefix + ' was not found. Aborting.')
  print('Looked in directory: ', input_path)
  quit()
#convert to numpy array and transpose
static_array = np.array(csv_list).astype(np.float)
if static_array.size == 0:
  print('Something wrong. No static echoes in static_array; size is 0')
  exit()
#First get the stdev of the buffer from data read in as csv_list

def get_stddev():
  #takes the global defined static_array and calculates the slope for 
  #each buffer size sub set
  #a dictionary object hold all pertinent data to dump to csv or xl
  #create the array to hold the slope values
  slopes_array = np.arange(static_array.size-buffsize)
  #We will not store all the buffer limits for each buffer size, but just the  current instance.
  x = np.arange(buffsize)
  for i in range(static_array.size-buffsize):
    buff = static_array[i:buffsize+i].T
    #get slope of each buff
    slope, intercept, r_value, p_value, std_err = stats.linregress(x, buff)
    slopes_array[i] = slope
    #get the 2 sigma slope of all the static slopes  
  std2_dev = -1*np.std(slopes_array)*2
  print('2 sigma slope of slopes: ', std2_dev)
  #get slope of all the pings.
  #reset the x range
  x = np.arange(static_array.size)
  slope, intercept, r_value, p_value, std_err = stats.linregress(x, static_array.T)
  print('slope of echoes: ', slope)
  data_dict["buffer_size"] = buffsize
  data_dict["buffer_slope_limit"] = std2_dev
  #also save the slope of all points
  data_dict["echo_slope"] = slope
  #plot the data?
  #get the buffer matrix and optimize it
  #get_buffer_matrix

def get_buffer_matrix(): 
  # set up 2D array with buffer elements in columns and buffer motion fill in rows.
  #There are n buffer elements, and n - 1 buffers to optimize
  #generate a buffer.
  buffsize =  data_dict["buffer_size"]
  #the matrix to be generated has n-1 buffer examples and each buffer contain n
  #where n is the buffer size elements in buffer index positions 0...n-1
  #fill each buffer limit (column) with the pre-calc number to optimize
  #b is the index of the particular buffer as (columns)
  #and serves as buffsize - b element to start filling with estimate.
  for b in range(1,buffsize):
    #first element to change from zero to value depends on buffer.
    #first element of buffer 1 should fill elem 9; 2nd is 8...etc. then
    #the rest of cells are filled with multiples of the first entry.
    #create a buffer to minimize   
    bx = np.arange(buffsize)
    buff = np.zeros(10)
    buff_index = b   
      #put element estimates in buffer
    for i in range(0,buffsize-1):
      #get the index that will be th first non zero element in buffer to be minimized.
      #get initial element guess for buff_index; values from buff_indexx to buff size will be calculated in op_fcn
      #guess must be in an np array.
      estmte = 0.0011 * buff_index ** 5 - 0.0681 * buff_index ** 4 + 1.622 * buff_index ** 3 - 18.438 * buff_index ** 2 + 100.92 ** buff_index - 224.72
      X0 = np.array([estmte])
      X0[0] = estmte
      #reverse buffer_index to create the buffer
      rev_buff_index = buffsize - buff_index
      ei = 1
      for e in range(rev_buff_index, buffsize):
        buff[e] = (ei)*estmte
        ei+=1 
      #add buffer to dictionary
      temp_dict[b] = buff
      #print('intial buffer: ', buff)
      #buff is the buffer with initial elementsWe know have the initial buffer to run with. we want to change the buffer values to get the result.
      #declare bounds
      bnds = (0.0, -500.0)
      slope_limit = data_dict['buffer_slope_limit']
      #print('slope_limit. X0 ', slope_limit, X0)
      elem = root(op_fcn, X0, args=(buff, bx, buff_index, slope_limit)) 
      #elem = minimize(op_fcn, X0, args=(buff, bx, buff_index, slope_limit), constraints=cons, method= 'SLSQP') 
    
    if elem.success:
      #print('elem found: ', elem.x)
      #add buffer to dictionary
      data_dict[b] = buff
      #calculate the velocity for each buffer.
      vel[b-1] = 0.5*elem.x/ping_interval*snd_vel*1000 #cm/s
      #used for debugging:
      #print('buffer number', b)
      #with np.printoptions(precision=3, suppress=True):
      #  print(buff)
    else:
      print('Failed to find root on buffer number ',b)
     
   
def op_fcn(x, buff, bx, bi, slope_limit): 
  # x is value to change; 
  # buff is buffer array, passed in from temp_dict; 
  # bx is x array for slope, as integers: 0...buffsize-1; 
  # buff_index is buffer element in buff to start filling estimate or root value 
  # the non root paramaters are passed in the callable fcn, and also
  # placed in args=....
  # we want to contrain the slope to the slope_limit value, which is calclated as
  # the 2 * standard deviation of all the static values collected and opened here
  # as , and find x which give that slope
  buffsize = buff.size
  bi = buffsize - bi
  ei = 1
  #replace the old estimate with new x that optimize set
  for e in range(bi, buffsize):
    buff[e] = ei*x
    ei+=1
  #print('buff: ',buff)
  #calculate the target function...slope
  slope, intercept, r_value, p_value, std_err = stats.linregress(bx, buff)
  temp_dict["slope"] = slope
  #print(temp_dict["slope"])
  return slope**2 - slope_limit**2 #this is the fcn constrained to zero

#print the completed buffer matrix
#Run it all
get_stddev()
get_buffer_matrix()
arr = np.array([data_dict[key] for key in range(1,buffsize)]).T
print('buffer matrix is:')
with np.printoptions(precision=2, suppress=True):
  print(arr)
print('corresponding velocity (cm/s):')
with np.printoptions(precision=2, suppress=True):
  print(vel)  
outfile = outfile + str(buffsize) + "." + out_file_type 
with open(outfile, "w", newline="") as outfile:  
  writer = csv.writer(outfile)
  writer.writerow(['buffer size', buffsize])
  writer.writerow(['slope limit', data_dict["buffer_slope_limit"]])
  writer.writerow(['Ping Interval', data_dict["ping_interval"]])
  writer.writerow(['velocity detection limit cm/s'])
  writer.writerow(vel)  
  writer.writerows(arr)
print("Finished writing ", outfile, " to ", input_path)

