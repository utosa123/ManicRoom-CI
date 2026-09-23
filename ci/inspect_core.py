"""Read-only Mach-O checks; no emulator execution or symbol-pointer calls."""
import hashlib,json,struct,sys
from pathlib import Path
from macho_export_decoder import export_trie
ORIGINAL='183159290d777d42a68c17f5f4d90d8b88f7aa0281e4788bad4e5954a6df940c'
UNSUPPORTED={'_retro_azahar_'+n for n in ('extension_version','install_cia','remove_amiibo')}
ADAPTERS={'_retro_azahar_'+n for n in ('set_keyboard_callback','keyboard_input','load_amiibo','is_searching_amiibo')}
ROOM={'_retro_azahar_room_'+n for n in ('api_version','snapshot','join','leave')}
def parse(path):
    b=Path(path).read_bytes();assert struct.unpack_from('<II',b)==(0xfeedfacf,0x100000c),'require thin arm64 Mach-O'
    assert struct.unpack_from('<I',b,12)[0]==6,'require MH_DYLIB'
    p=32;deps=[];exports=None;platform=None;identity=None
    for _ in range(struct.unpack_from('<I',b,16)[0]):
        cmd,size=struct.unpack_from('<II',b,p);assert size>=8 and p+size<=len(b)
        if cmd==0x80000033:
            offset,length=struct.unpack_from('<II',b,p+8);assert offset+length<=len(b);exports=export_trie(b[offset:offset+length])
        if cmd in (0x22,0x80000022):
            offset,length=struct.unpack_from('<II',b,p+40)
            if length:exports=export_trie(b[offset:offset+length])
        if cmd==0x32:platform=struct.unpack_from('<I',b,p+8)[0]
        if cmd==0x25:platform=2 # LC_VERSION_MIN_IPHONEOS; arm64 slice is already mandatory
        if cmd in (0xc,0x18|0x80000000,0x1f|0x80000000,0x20,0x23|0x80000000,0xd):
            off=struct.unpack_from('<I',b,p+8)[0];assert 24<=off<size
            name=b[p+off:b.index(0,p+off,p+size)].decode()
            if cmd==0xd:identity=name
            else:deps.append({'name':name,'command':hex(cmd)})
        p+=size
    assert platform==2,'require iOS device, not simulator/macOS'
    assert exports is not None,'missing export trie'
    return {'sha256':hashlib.sha256(b).hexdigest(),'exports':exports,'dependencies':deps,'platform':platform,'id':identity}
def validate(original,candidate,phase):
    assert original['sha256']==ORIGINAL and candidate['sha256']!=ORIGINAL
    names={e['name'] for e in candidate['exports']}
    if phase=='binary':
        assert candidate['id']=='@rpath/azahar.libretro.framework/azahar.libretro'
        assert not any(e['flags']&8 for e in candidate['exports']),'re-export requires explicit review'
        for d in candidate['dependencies']:
            assert d['name'].startswith(('/System/Library/','/usr/lib/')),'unpackaged non-system dependency: '+d['name']
    elif phase=='custom':
        expected={e['name'] for e in original['exports'] if e['name'].startswith('_retro_')}
        assert expected-names==UNSUPPORTED, 'unexpected missing legacy APIs or unsupported API was added'
        assert ADAPTERS<=names and '_retro_manic_experiment_api_version' in names
    elif phase=='room':assert ROOM<=names,'missing Room exports'
    else:raise ValueError(phase)
def main():
    phase,original,candidate,out=sys.argv[1:];target=Path(out);target.mkdir(parents=True,exist_ok=True)
    old=parse(original);new=parse(candidate)
    (target/'candidate-macho.json').write_text(json.dumps(new,indent=2))
    validate(old,new,phase)
    (target/(phase+'-PASS.json')).write_text(json.dumps({'phase':phase,'result':'success','runtime_verified':False,'sha256':new['sha256']},indent=2))
    print('PASS',phase,'static check; no runtime compatibility claim')
if __name__=='__main__':main()
