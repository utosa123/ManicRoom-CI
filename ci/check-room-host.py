"""Prepared-source gates for ABI 2 host lifecycle and concise Room UI.

These checks cannot replace the iOS build or a device-to-PC Room test.
"""
import argparse
from pathlib import Path

parser = argparse.ArgumentParser()
parser.add_argument('--azahar', type=Path, default=Path('src/azahar'))
parser.add_argument('--manic', type=Path, default=Path('src/ManicEMU'))
parser.add_argument('--libretro', type=Path, default=Path('src/ManicEMU/Dependencies/Libretro'))
args = parser.parse_args()
az = args.azahar/'src/citra_libretro'
manic = args.manic/'ManicEmu/ManicEmu/Sources/Business'
bridge = args.libretro
session = (az/'room_session.cpp').read_text(encoding='utf-8')
core_header = (az/'room_api.h').read_text(encoding='utf-8')
ui_header = (bridge/'pkg/apple/ManicEMU/AzaharRoomABI.h').read_text(encoding='utf-8')
exports = (az/'libretro.osx.def').read_text(encoding='utf-8')
driver = (bridge/'ui/drivers/LibretroCore.m').read_text(encoding='utf-8')
driver_header = (bridge/'ui/drivers/cocoa/LibretroCore.h').read_text(encoding='utf-8')
view = (manic/'OnlinePlay/Views/AzaharRoomView.swift').read_text(encoding='utf-8')
option = (manic/'Games/Models/GameOption.swift').read_text(encoding='utf-8')

apis = ('host', 'close', 'is_hosting')
for name in apis:
    symbol = 'retro_azahar_room_' + name
    assert symbol in session and symbol in core_header and symbol in ui_header
    assert '_' + symbol in exports and symbol in driver
assert 'retro_azahar_room_api_version() { return 2; }' in session
assert 'version() == 2' in driver
assert 'Network::GetRoom().lock()' in session
assert 'server->Create("Multiplayer Room"' in session
assert 'std::make_unique<Network::VerifyUser::NullBackend>()' in session
assert '"127.0.0.1"' in session
assert 'server->Destroy()' in session
assert 'member->Leave(true);' in session
assert 'return host_mode && snapshot.state == AZ_ROOM_JOINED ? 1 : 0;' in session
for lifecycle in ('void GameStopped()', 'void Shutdown()'):
    assert 'StopWorker();' in session.split(lifecycle, 1)[1].split('\n}', 1)[0]
assert '[self leaveAzaharRoom];' in driver.split('- (void)azaharRoomDidEnterBackground:', 1)[1].split('\n}', 1)[0]
assert '- (NSInteger)hostAzaharRoom:' in driver_header
assert '- (void)closeAzaharRoom;' in driver_header
assert 'Multiplayer Room' in view and 'Multiplayer Room' in option
assert 'UISegmentedControl(items: ["参加", "作成"])' in view
assert 'hostAzaharRoom(' in view and 'closeAzaharRoom()' in view
assert '同じWi-FiのPC Azaharルームへ接続します' not in view
assert '公開候補コアは実験用です' not in view
assert 'Azahar Room (3DS LAN)' not in view and 'Azahar Room (3DS LAN)' not in option
print('PASS Room host ABI, lifecycle, export and UI source gates (device untested)')
