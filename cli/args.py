import argparse
import sys

def parse_args():
    if len(sys.argv) > 1 and sys.argv[1] in ("-help", "-h", "--help") and len(sys.argv) == 2:
        print("USAGE: ./client -n <team> -p <port> [-h <hostname>]")
        sys.exit(0)
    p = argparse.ArgumentParser(add_help=False)
    p.add_argument("-n", required=True)
    p.add_argument("-p", type=int, required=True)
    p.add_argument("-h", "--hostname", default="localhost")
    args, _ = p.parse_known_args()
    return args
