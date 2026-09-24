"""Source preservation checks only; not a device/JIT test."""
import hashlib,json
from pathlib import Path
root=Path(__file__).resolve().parent.parent
expected=json.loads((root/'ci/diag-protected.json').read_text())
for name,digest in expected['sha256_lf'].items():
    assert hashlib.sha256((root/name).read_bytes().replace(b'\r\n',b'\n')).hexdigest()==digest,name
core=(root/'changes/azahar-experimental/src/citra_libretro/citra_libretro.cpp').read_text(encoding='utf8')
assert all(x in core for x in ('Span diag("retro_init")','Span diag("retro_load_game")','Core.System.Load ENTER','Core.System.Load RETURN','Core.System.RunLoop ENTER','Core.System.RunLoop RETURN','Span diag("context_reset")'))
assert 'os_log_with_type' in (root/'changes/azahar-experimental/src/citra_libretro/room_c_diag.h').read_text()
out=root/'logs';out.mkdir(exist_ok=True)
(out/'diag-scope.json').write_text(json.dumps({'protected_files':len(expected['sha256_lf']),'baseline':expected['baseline'],'result':'pass','device_tested':False},indent=2))
print('PASS diagnostic markers and unchanged Room/CIA/adapter/gate source hashes')
