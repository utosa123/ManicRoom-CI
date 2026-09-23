"""Read-only, bounded Mach-O investigation. Never loads or executes the image.
Python 3.12; pip install lief==0.16.6 capstone==5.0.3
Usage: python analyze_original_core.py BINARY NEW_OR_EXISTING_LOG_DIR [--deps DIR]
All addresses are unslid image VM addresses, NOT callable runtime pointers.
"""
import argparse, bisect, collections, csv, hashlib, json, re, struct, sys
from pathlib import Path

EXPECTED='183159290d777d42a68c17f5f4d90d8b88f7aa0281e4788bad4e5954a6df940c'
def uleb(data,pos):
    value=0
    for shift in range(0,70,7):
        x=data[pos];pos+=1;value|=(x&127)<<shift
        if not x&128:return value,pos
    raise ValueError('invalid ULEB')
def cstr(data,pos):
    end=data.index(0,pos)
    return data[pos:end].decode('utf8',errors='replace'),end+1
def export_trie(data):
    """Independent decoder; handles regular/TLV/absolute/re-export/resolver terminals."""
    found=[]
    def walk(offset,name,ancestors):
        if offset in ancestors or offset>=len(data):raise ValueError('invalid trie')
        terminal,pos=uleb(data,offset);end=pos+terminal
        if terminal:
            flags,pos=uleb(data,pos);row={'name':name,'flags':flags,'node_offset':offset}
            if flags&8:
                row['ordinal'],pos=uleb(data,pos);row['import_name'],pos=cstr(data,pos)
            else:
                row['address'],pos=uleb(data,pos)
                if flags&16:row['resolver'],pos=uleb(data,pos)
            if pos!=end:raise ValueError('unconsumed terminal')
            found.append(row)
        pos=end;count=data[pos];pos+=1
        for _ in range(count):
            label,pos=cstr(data,pos);child,pos=uleb(data,pos)
            walk(child,name+label,ancestors|{offset})
    walk(0,'',set())
    return sorted(found,key=lambda x:x['name'])
