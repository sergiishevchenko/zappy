#!/usr/bin/env python3

import sys

from args import parse_args
from network import ZappyClient
from strategy import Strategy

def main():
    args = parse_args()
    client = ZappyClient(args.hostname, args.p, args.n)
    if not client.connect():
        sys.exit(84)
    strat = Strategy(client)
    strat.run()

if __name__ == "__main__":
    main()
