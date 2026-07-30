#!/bin/bash

perf record --call-graph dwarf ./prgm.x86_64
hotspot
