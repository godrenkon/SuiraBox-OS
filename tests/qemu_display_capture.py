#!/usr/bin/env python3
"""Boot a file-backed CI fixture, wait for guest proof, and capture through QMP."""
import json
from pathlib import Path
import socket
import subprocess
import sys
import time


def run(build: Path, boot_text: bool = False, keyboard: bool = False, shell: bool = False, browser: bool = False, paging: bool = False) -> None:
    build = build.resolve()
    serial = build / "display.log"
    qmp = build / "qmp.sock"
    # Only this tool's disposable socket is removed; never a user disk path.
    qmp.unlink(missing_ok=True)
    serial.unlink(missing_ok=True)
    marker = ("Userspace: keyboard demo ready" if keyboard else "Userspace: shell ready" if boot_text or shell or browser or paging
              else "Userspace: display surface present and rejection lifecycle OK")
    deadline = time.monotonic() + (45 if browser or paging else 30)
    with (build / "qemu-host.log").open("wb") as host_log:
        process = subprocess.Popen([
            "qemu-system-x86_64", "-m", "256M", "-boot", "d",
            "-drive", f"file={build / 'runtime.img'},format=raw,if=ide,index=0,media=disk,cache=writeback",
            "-cdrom", str(build / "suirabox.iso"), "-serial", f"file:{serial}",
            "-display", "none", "-no-reboot", "-no-shutdown",
            "-qmp", f"unix:{qmp},server=on,wait=off",
        ], stdin=subprocess.DEVNULL, stdout=host_log, stderr=subprocess.STDOUT)
        try:
            while True:
                log = serial.read_text(errors="replace") if serial.exists() else ""
                if "FAILED" in log or "Exception:" in log:
                    raise RuntimeError("guest failed; inspect display.log")
                if (marker in log and
                        "Userspace: concurrent child processes completed" in log):
                    break
                if process.poll() is not None or time.monotonic() > deadline:
                    raise RuntimeError("QEMU display proof did not finish before timeout")
                time.sleep(0.1)
            with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as connection:
                connection.settimeout(5)
                connection.connect(str(qmp))
                with connection.makefile("rwb") as stream:
                    greeting = json.loads(stream.readline())
                    if "QMP" not in greeting:
                        raise RuntimeError("missing QMP greeting")
                    def command(name, arguments=None):
                        request = {"execute": name, "id": name}
                        if arguments is not None:
                            request["arguments"] = arguments
                        stream.write(json.dumps(request).encode() + b"\r\n")
                        stream.flush()
                        while True:
                            line = stream.readline()
                            if not line:
                                raise RuntimeError("QMP closed before response")
                            response = json.loads(line)
                            if response.get("id") != name:
                                continue
                            if "error" in response:
                                raise RuntimeError(f"QMP {name} failed: {response['error']}")
                            return
                    command("qmp_capabilities")
                    def wait_marker(expected):
                        while True:
                            log = serial.read_text(errors="replace")
                            if "FAILED" in log or "Exception:" in log:
                                raise RuntimeError("guest input failure")
                            if expected in log:
                                return
                            if process.poll() is not None or time.monotonic() > deadline:
                                raise RuntimeError(f"missing guest marker: {expected}")
                            time.sleep(0.01)
                    def key(code, down):
                        command("input-send-event", {"events": [{"type":"key", "data": {
                            "down":down, "key":{"type":"qcode", "data":code}}}]})
                    if shell or browser or paging:
                        def capture(name):
                            command("stop")
                            command("screendump", {"filename": str(build / f"{name}.ppm")})
                            command("screendump", {"filename": str(build / f"{name}.png"), "format": "png"})
                            command("cont")
                        wait_marker("Userspace: shell frame 1 Home")
                        capture("home")
                        stages = [("f2", "Files /boot", "boot-files"),
                                  ("d", "Files /disk", "disk-files"),
                                  ("r", "Files /disk", "disk-refresh"),
                                  ("f3", "Settings", "settings"),
                                  ("tab", "Home", "home-cycle"),
                                  ("shift-tab", "Settings", "settings-reverse"),
                                  ("esc", "Home", "home-escape"),
                                  ("f2", "Files /disk", "disk-return"),
                                  ("b", "Files /boot", "boot-return")]
                        if browser:
                            stages = [("f2","Files /boot","boot-files"),
                                      ("d","Files /disk","disk-files"),
                                      ("down","Files /disk","select-saves"),
                                      ("ret","Files /disk/SAVES","saves"),
                                      ("ret","Files /disk/SAVES/WORLDS","worlds"),
                                      ("ret","Files /disk/SAVES/WORLDS preview","level-preview"),
                                      ("esc","Files /disk/SAVES/WORLDS","worlds-return"),
                                      ("backspace","Files /disk/SAVES","saves-return"),
                                      ("down","Files /disk/SAVES","select-readme"),
                                      ("ret","Files /disk/SAVES preview","readme-preview"),
                                      ("esc","Files /disk/SAVES","readme-return"),
                                      ("down","Files /disk/SAVES","select-empty"),
                                      ("ret","Files /disk/SAVES preview","empty-preview"),
                                      ("esc","Files /disk/SAVES","empty-return"),
                                      ("down","Files /disk/SAVES","select-long"),
                                      ("ret","Files /disk/SAVES preview","long-preview"),
                                      ("esc","Files /disk/SAVES","long-return"),
                                      ("down","Files /disk/SAVES","select-binary"),
                                      ("ret","Files /disk/SAVES preview","binary-preview"),
                                      ("esc","Files /disk/SAVES","binary-return"),
                                      ("backspace","Files /disk","root-return"),
                                      ("b","Files /boot","boot-return"),
                                      ("ret","Files /boot preview","elf-preview"),
                                      ("esc","Files /boot","elf-return"),
                                      ("esc","Home","home-return")]
                        if paging:
                            stages = [("f2","Files /boot","boot-files"),
                                      ("d","Files /disk","disk-files"),
                                      ("down","Files /disk","select-many"),
                                      ("ret","Files /disk/MANY","page-one"),
                                      ("pgdn","Files /disk/MANY","page-two"),
                                      ("ret","Files /disk/MANY preview","file13-preview"),
                                      ("esc","Files /disk/MANY","file13-return"),
                                      ("pgdn","Files /disk/MANY","page-three"),
                                      ("ret","Files /disk/MANY preview","file25-preview"),
                                      ("esc","Files /disk/MANY","file25-return"),
                                      ("down","Files /disk/MANY","select26"),
                                      ("down","Files /disk/MANY","select27"),
                                      ("up","Files /disk/MANY","select26-return"),
                                      ("pgup","Files /disk/MANY","page-two-return"),
                                      ("up","Files /disk/MANY","page-one-last"),
                                      ("ret","Files /disk/MANY preview","file12-preview"),
                                      ("esc","Files /disk/MANY","file12-return"),
                                      ("down","Files /disk/MANY","page-two-edge"),
                                      ("r","Files /disk/MANY","page-two-refresh"),
                                      ("backspace","Files /disk","root-return"),
                                      ("b","Files /boot","boot-return"),
                                      ("esc","Home","home-return")]
                        for number, (code, view, name) in enumerate(stages, 2):
                            if code == "shift-tab":
                                key("shift", True); key("tab", True); key("tab", False); key("shift", False)
                            else:
                                key(code, True); key(code, False)
                            wait_marker(f"Userspace: shell frame {number} {view}\n")
                            capture(name)
                    if keyboard:
                        command("stop")
                        command("screendump", {"filename": str(build / "before.ppm")})
                        command("screendump", {"filename": str(build / "before.png"), "format": "png"})
                        command("cont")
                        events = [("shift",True),("a",True),("a",False),("shift",False),
                                  ("b",True),("b",False),("backspace",True),("backspace",False),
                                  ("caps_lock",True),("caps_lock",False),("c",True),("c",False),
                                  ("caps_lock",True),("caps_lock",False),("right",True),("right",False),
                                  ("ctrl_r",True),("ctrl_r",False),("alt_r",True),("alt_r",False),
                                  ("ret",True),("ret",False)]
                        for index, (key, down) in enumerate(events, 1):
                            command("input-send-event", {"events": [{"type":"key", "data": {
                                "down":down, "key":{"type":"qcode", "data":key}}}]})
                            wait_marker(f"Userspace: keyboard event {index:02d} OK")
                        wait_marker("Userspace: PS/2 IRQ keyboard editing and rejection lifecycle OK")
                    command("stop")
                    command("screendump", {"filename": str(build / "screen.ppm")})
                    command("screendump", {"filename": str(build / "screen.png"), "format": "png"})
                    command("quit")
            if process.wait(timeout=5) != 0:
                raise RuntimeError("QEMU exited unsuccessfully after capture")
        except Exception:
            # Preserve useful CI diagnostics even if QMP or the guest fails.
            if serial.exists():
                print(serial.read_text(errors="replace")[-16000:], file=sys.stderr)
            print((build / "qemu-host.log").read_text(errors="replace")[-4000:], file=sys.stderr)
            raise
        finally:
            if process.poll() is None:
                process.terminate()
                try:
                    process.wait(timeout=5)
                except subprocess.TimeoutExpired:
                    process.kill()
                    process.wait(timeout=5)
    print("QEMU display captured after completed guest and IPC lifecycle")


if __name__ == "__main__":
    run(Path(sys.argv[1]), "--boot-text" in sys.argv[2:], "--input-proof" in sys.argv[2:], "--shell" in sys.argv[2:], "--browser" in sys.argv[2:], "--paging" in sys.argv[2:])
