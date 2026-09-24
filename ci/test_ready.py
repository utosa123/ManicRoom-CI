import copy,json,sys,unittest
from pathlib import Path
from verify_b import validate_run,validate_attestation,ORIGINAL
from inspect_core import parse,validate,ADAPTERS,ROOM,UNSUPPORTED
BINARY=Path(sys.argv.pop(1));EXPECTED=Path(sys.argv.pop(1))
class GateTests(unittest.TestCase):
    def setUp(self):
        self.run={'id':123,'repository':{'full_name':'owner/project'},'head_sha':'a'*40,'status':'completed','conclusion':'success','event':'workflow_dispatch','path':'.github/workflows/room-experiment.yml'}
        self.att={'schema':1,'mode':'ui','archive_success':True,'communication_verified':False,'repository':'owner/project','head_sha':'a'*40,'run_id':'123','original_sha256':ORIGINAL,'bundle_id':'local.manicroom.ui'}
    def test_valid_B(self):
        validate_run(self.run,'owner/project','a'*40,'123');validate_attestation(self.att,'owner/project','a'*40,'123')
    def test_reject_failed_pending_stale_foreign_runs(self):
        for key,value in [('conclusion','failure'),('status','in_progress'),('head_sha','b'*40),('event','push'),('path','another.yml'),('id',124),('repository',{'full_name':'other/repo'})]:
            r=copy.deepcopy(self.run);r[key]=value
            with self.assertRaises(AssertionError):validate_run(r,'owner/project','a'*40,'123')
    def test_reject_forged_incompatible_attestation(self):
        for key,value in [('archive_success',False),('mode','experimental'),('communication_verified',True),('head_sha','b'*40),('original_sha256','changed'),('bundle_id','com.aoshuang.manicemu')]:
            a=dict(self.att);a[key]=value
            with self.assertRaises(AssertionError):validate_attestation(a,'owner/project','a'*40,'123')
class MachOTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):cls.old=parse(BINARY)
    def candidate_fixture(self):
        names=({e['name'] for e in self.old['exports']}-UNSUPPORTED)|ROOM|{'_retro_manic_experiment_api_version','_retro_manic_install_update_cia_v1'}
        return {'sha256':'synthetic-fixture-not-a-core','exports':[{'name':n,'flags':0,'address':1} for n in names],
                'dependencies':[{'name':'/usr/lib/libc++.1.dylib','command':'0xc'}],
                'platform':2,'id':'@rpath/azahar.libretro.framework/azahar.libretro'}
    def test_original_trie_matches_independently_verified_log(self):
        self.assertEqual(self.old['sha256'],ORIGINAL)
        self.assertEqual(self.old['exports'],json.loads(EXPECTED.read_text()))
    def test_synthetic_expected_contract(self):
        c=self.candidate_fixture()
        for phase in ('binary','custom','room'):validate(self.old,c,phase)
    def test_missing_adapters_or_room_rejected(self):
        for n in ADAPTERS|ROOM:
            c=self.candidate_fixture();c['exports']=[e for e in c['exports'] if e['name']!=n]
            with self.assertRaises(AssertionError):validate(self.old,c,'room' if n in ROOM else 'custom')
    def test_unsupported_apis_must_not_be_faked(self):
        for n in UNSUPPORTED:
            c=self.candidate_fixture();c['exports'].append({'name':n,'flags':0,'address':1})
            with self.assertRaises(AssertionError):validate(self.old,c,'custom')
    def test_non_system_dependency_and_reexport_rejected(self):
        c=self.candidate_fixture();c['dependencies'].append({'name':'/opt/homebrew/lib/unbundled.dylib'})
        with self.assertRaises(AssertionError):validate(self.old,c,'binary')
        c=self.candidate_fixture();c['exports'][0]['flags']=8
        with self.assertRaises(AssertionError):validate(self.old,c,'binary')
unittest.main(verbosity=2)
