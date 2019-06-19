def main(argv):
        # Here goes parsing input arguments.
        try:
                opts, args = getopt.getopt(argv,"hiva:s:",["inverted","verbose","i2caddr=","sensor="])
        except getopt.GetoptError:
                print ('usage:getdata.py -a <hex i2c address> -s <sensor or service request> [-v] [-i]')
                sys.exit(2)
        # default flags
        verbose = 0
        inverted = 0
        for opt, arg in opts:
                if opt == '-h':
                        print ('usage:getdata.py -a <hex i2c address> -s <data> [-v] [-i] \n Script is used to send requests to i2c address (-a parameter) with some data in it (-s parameter) and this data is limited w
ith 1 byte (0-255). \n So, -a and -s parameters are mandatory. Other is optional. \n -v shows verbose output, -i shows inverted output (simply 255 - data)')
                        sys.exit()
                elif opt in ("-i", "--inverted"):
                        inverted = 1
                elif opt in ("-v", "--verbose"):
                        verbose = 1
                elif opt in ("-a", "--i2caddr"):
                        address = arg
                elif opt in ("-s", "--sensor"):
                        send_int = arg

        if verbose == 1:
                print ("Polling sensor ",send_int," on i2c address ",address)
        bus.write_byte(int(address), int(send_int))
        time.sleep(1)
        rcv_byte = bus.read_byte(int(address))
        if verbose == 1 :
                if inverted == 1 :
                        print ("Received data ", 255 - rcv_byte)
                else:
                        print ("Received data ",rcv_byte)
        else:
                if inverted == 1 :
                        print (255 - rcv_byte)
                else:
                        print (rcv_byte)

main(sys.argv[1:])
