import os
import sys
import getopt
import csv

def main(argv):
   inputfile = ''
   outputfile = ''
   downsample = 1
   multi = False
   try:
      opts, args = getopt.getopt(argv,"hi:o:v:d:m:",[""])
   except getopt.GetoptError:
      print('python panicle_batch.py -i <inputfile> -o <outputfile> -d <downsampling rate:optional> -m (1 or 0)')
      sys.exit(2)
   for opt, arg in opts:
      if opt == '-h':
         print('python panicle_batch.py -i <inputfile> -o <outputfile> -d <downsampling rate:optional> -m (1 or 0)')
         sys.exit()
      elif opt == '-i':
         inputfile = arg
      elif opt == '-o':
         outputfile = arg
      elif opt == '-d':
         downsample = float(arg)
   print('Input directory is ', inputfile)
   print('Output directory is ', outputfile)
   print('Downsampling rate is ', downsample)
   for filename in os.listdir(inputfile):
      if not filename.endswith(".dat") and not filename.endswith(".raw"):
         outputStr = outputfile + filename
         print("Processing ", inputfile+filename, outputStr)
         command = "Panicle --in " + inputfile+filename + " --out " + outputStr + " --d " + str(downsample)

         os.system(command)
      if filename.endswith(".raw"):
         print("Processing ", inputfile+filename)
         datFile = inputfile+filename[0:-3] + "dat"
         outputStr = outputfile + filename[0:-4]
         command = "Panicle --in " + inputfile+filename + " --dat " + datFile + " --out " + outputStr + " --d " + str(downsample)
         os.system(command)


if __name__ == "__main__":
   main(sys.argv[1:])