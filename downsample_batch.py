import os
import sys
import getopt
import csv

def main(argv):
   inputfile = ''
   outputfile = ''
   downsample = 6
   multi = 0
   costType = 0
   try:
      opts, args = getopt.getopt(argv,"hi:o:v:d:m:c:",[""])
   except getopt.GetoptError:
      print('python toporoot_batch.py -i <inputfile> -o <outputfile> -d <downsampling rate>')
      sys.exit(2)
   for opt, arg in opts:
      if opt == '-h':
         print('python toporoot_batch.py -i <inputfile> -o <outputfile> -d <downsampling rate>')
         sys.exit()
      elif opt == '-i':
         inputfile = arg
      elif opt == '-o':
         outputfile = arg
      elif opt == '-d':
         downsample = float(arg)
      elif opt == '-m':
         multi = int(arg)
      elif opt == '-c':
         costType = int(arg)
   print('Input directory is ', inputfile)
   print('Output directory is ', outputfile)
   print('Downsampling rate is ', downsample)
   for filename in os.listdir(inputfile):
      if not filename.endswith(".dat") and not filename.endswith(".raw"):
         outputStr = outputfile + filename
         print(inputfile+filename)
         print(outputStr)
         print(outputfile + "traits.csv")
         os.system("python downsample.py -i " + inputfile+filename + " -o " + outputStr + " -d " + str(downsample) )
      if filename.endswith(".raw"):
         print("Processing ", inputfile+filename)
         datFile = inputfile+filename[0:-3] + "dat"
         outputStr = outputfile + filename[0:-4]
         os.system("python downsample.py -i " + inputfile+filename + " -x " + datFile + " -o " + outputStr + " -d " + str(downsample))




if __name__ == "__main__":
   main(sys.argv[1:])