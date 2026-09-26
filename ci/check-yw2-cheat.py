"""Static source gates for the experimental YW2 and Cheat additions.

The iOS build checks compilation; game and Room behavior still require device tests.
"""
import argparse
import json
from pathlib import Path

parser = argparse.ArgumentParser()
parser.add_argument("--azahar", type=Path, default=Path("src/azahar"))
parser.add_argument("--manic", type=Path, default=Path("src/ManicEMU"))
args = parser.parse_args()


def source(root, name):
    return (root / name).read_text(encoding="utf-8")


def require(name, condition):
    if not condition:
        raise AssertionError(name)
    checks.append(name)


checks = []
kernel_h = source(args.azahar, "src/core/hle/kernel/kernel.h")
kernel_cpp = source(args.azahar, "src/core/hle/kernel/kernel.cpp")
svc = source(args.azahar, "src/core/hle/kernel/svc.cpp")
process = source(args.azahar, "src/core/hle/kernel/process.cpp")
uds = source(args.azahar, "src/core/hle/service/nwm/nwm_uds.cpp")
uds_h = source(args.azahar, "src/core/hle/service/nwm/nwm_uds.h")

title_ids = [
    "0x0004000000155100", "0x000400000012F900", "0x000400000012F800",
    "0x000400000019A900", "0x000400000019AA00", "0x00040000001B2700",
    "0x000400000019AB00", "0x000400000019AC00", "0x00040000001B2900",
    "0x000400000019B000", "0x000400000019B100", "0x00040000001B2800",
    "0x000400000019AE00", "0x000400000019AF00", "0x00040000001B2A00",
]
require("YW2 retail title allowlist", "IsYoKaiWatch2Title" in kernel_h and all(x in kernel_h for x in title_ids))
require("NWM 0x0021 handler", '{0x0021, &NWM_UDS::SetProbeResponseParam' in uds and
        "void NWM_UDS::SetProbeResponseParam" in uds and "void SetProbeResponseParam" in uds_h)
require("UDS host/client worker registration", all(x in uds for x in (
    "RegisterYW2UDSWorkerOrderingWorkaround(ctx.ClientThread(), 0x001D)",
    "RegisterYW2UDSWorkerOrderingWorkaround(ctx.ClientThread(), 0x001E)")))
require("YW2 identity and first-poll state", all(x in kernel_cpp + kernel_h for x in (
    "IsYoKaiWatch2Title(process->codeset->program_id)", "first_poll_consumed",
    "registered_process != process", "registered_thread != thread",
    "state.first_poll_consumed = true")))
require("YW2 timeout/dead-worker restriction", all(x in svc for x in (
    "!actual_should_wait && nano_seconds == 0", "HandleType::Thread",
    "target_thread->status == ThreadStatus::Dead", "IsYoKaiWatch2Title",
    "TryUseYW2UDSWorkerOrderingWorkaround")))
require("YW2 process-exit cleanup", "ClearYW2UDSWorkerOrderingWorkaround(process)" in process)

cheats_h = source(args.azahar, "src/core/cheats/cheats.h")
cheats_cpp = source(args.azahar, "src/core/cheats/cheats.cpp")
libretro = source(args.azahar, "src/citra_libretro/citra_libretro.cpp")
require("Separate libretro cheat registry", all(x in cheats_h + cheats_cpp for x in (
    "std::map<unsigned, std::shared_ptr<CheatBase>> libretro_cheats",
    "SetLibretroCheat(unsigned index", "ResetLibretroCheats()",
    "libretro_cheats.clear()", "cheats_list")))
require("Real libretro GatewayCheat adapter", all(x in libretro for x in (
    "void retro_cheat_reset() {", "ResetLibretroCheats()", "void retro_cheat_set(unsigned index",
    "std::replace(gateway_code.begin(), gateway_code.end(), '+', '\\n')",
    "std::make_shared<Cheats::GatewayCheat>", "cheat->SetEnabled(enabled)",
    "engine.SetLibretroCheat(index, std::move(cheat))")))
require("Cheats execute during Room", all(x in cheats_cpp for x in (
    "ScheduleEvent(run_interval_ticks - cycles_late, event)",
    "for (const auto& [index, cheat] : libretro_cheats)", "cheat->Execute(system, process_id)")) and
        "RoomSession::Active()" not in cheats_cpp and
        "ResetLibretroCheats()" not in cheats_cpp.split("void CheatEngine::RunCallback", 1)[1])
require("Core allows cheat edits during Room", all(
    "RoomSession::Active()" not in libretro.split(marker, 1)[1].split("\n}", 1)[0]
    for marker in ("void retro_cheat_reset() {", "void retro_cheat_set(unsigned index")))

option = source(args.manic, "ManicEmu/ManicEmu/Sources/Business/Games/Models/GameOption.swift")
perform = source(args.manic, "ManicEmu/ManicEmu/Sources/Business/Games/Models/GameOptionPerform.swift")
play = source(args.manic, "ManicEmu/ManicEmu/Sources/Business/Play/VIewControllers/PlayViewController.swift")
view = source(args.manic, "ManicEmu/ManicEmu/Sources/Business/CheatCode/Views/CheatCodeListView.swift")
require("C-only cheat UI without Room gate", all(x in option for x in (
    '"ManicRoomBuildMode"', "if !game.isAzahar3DS {",
    "allOptions.remove(.cheatCode)")) and "case .cheatCode:" in perform and
        "azaharRoomActive()" not in perform.split("case .cheatCode:", 1)[1].split("case .", 1)[0] and
        "roomCheatsLocked" not in view)
require("C-only direct libretro cheat route", all(x in play for x in (
    "if experimentalAzahar", "resetCheatCode()", "addCheatCode(coreCode",
    'replacingOccurrences(of: "\\n", with: "+")')) and
        "experimentalAzahar && LibretroCore.sharedInstance().azaharRoomActive()" not in play)
require("Experimental state/speed restrictions retained", all(x in option for x in (
    ".saveState", ".quickLoadState", ".stateList", ".fastForward", ".rewind", ".slowMotion")) and
        "#ifdef MANIC_ROOM_EXPERIMENT" in libretro and "size_t retro_serialize_size()" in libretro)

out = Path("logs")
out.mkdir(exist_ok=True)
(out / "yw2-cheat-static.json").write_text(json.dumps({
    "result": "pass", "checks": checks, "device_tested": False,
    "source_commit": "2b9131b881a7600089117fd5df55e1de57d18a6b",
}, indent=2) + "\n", encoding="utf-8")
print(f"PASS {len(checks)} YW2/Cheat source gates; device behavior remains untested")
