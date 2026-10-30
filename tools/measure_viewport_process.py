"""Read toolkit CPU time and Windows GPU counters; never changes process state.

Usage: python tools/measure_viewport_process.py LABEL [SECONDS]
CPU one-core 100% means one logical processor is fully occupied.
"""
import ctypes as c
from ctypes import wintypes as w
import json
import os
import sys
import time

k = c.WinDLL('kernel32', use_last_error=True)
k.OpenProcess.argtypes = [w.DWORD, w.BOOL, w.DWORD]
k.OpenProcess.restype = w.HANDLE
k.CloseHandle.argtypes = [w.HANDLE]
k.GetProcessTimes.argtypes = [w.HANDLE] + [c.POINTER(c.c_ulonglong)] * 4
k.QueryFullProcessImageNameW.argtypes = [w.HANDLE, w.DWORD, w.LPWSTR, c.POINTER(w.DWORD)]
k.K32EnumProcesses.argtypes = [c.POINTER(w.DWORD), w.DWORD, c.POINTER(w.DWORD)]

def processes():
    ids = (w.DWORD * 8192)()
    size = w.DWORD()
    if not k.K32EnumProcesses(ids, c.sizeof(ids), c.byref(size)):
        raise c.WinError(c.get_last_error())
    found = []
    for pid in ids[:size.value // 4]:
        h = k.OpenProcess(0x1000, False, pid)
        if not h:
            continue
        path = c.create_unicode_buffer(32768)
        length = w.DWORD(len(path))
        ok = k.QueryFullProcessImageNameW(h, 0, path, c.byref(length))
        if ok and path.value.lower().endswith('toyxyz_openvr_toolkit.exe'):
            found.append((pid, h, path.value))
        else:
            k.CloseHandle(h)
    return found

def cpu(h):
    values = [c.c_ulonglong() for _ in range(4)]
    if not k.GetProcessTimes(h, *(c.byref(v) for v in values)):
        raise c.WinError(c.get_last_error())
    return (values[2].value + values[3].value) / 1e7

items = processes()
if not items:
    raise SystemExit('No toolkit process found')
if len(sys.argv) < 2:
    raise SystemExit('Usage: measure_viewport_process.py LABEL [SECONDS]')
pdh = c.WinDLL('pdh')
query, counter = w.HANDLE(), w.HANDLE()
pdh.PdhOpenQueryW.argtypes = [w.LPCWSTR, c.c_size_t, c.POINTER(w.HANDLE)]
pdh.PdhAddEnglishCounterW.argtypes = [w.HANDLE, w.LPCWSTR, c.c_size_t, c.POINTER(w.HANDLE)]
pdh.PdhCollectQueryData.argtypes = [w.HANDLE]
pdh.PdhGetFormattedCounterArrayW.argtypes = [w.HANDLE, w.DWORD,
    c.POINTER(w.DWORD), c.POINTER(w.DWORD), c.c_void_p]
pdh.PdhCloseQuery.argtypes = [w.HANDLE]
class Value(c.Structure):
    _fields_ = [('status', w.DWORD), ('value', c.c_double)]
class Item(c.Structure):
    _fields_ = [('name', w.LPWSTR), ('formatted', Value)]
def check(result):
    if result:
        raise RuntimeError(f'PDH error {result & 0xffffffff:#x}')
check(pdh.PdhOpenQueryW(None, 0, c.byref(query)))
check(pdh.PdhAddEnglishCounterW(query,
    r'\GPU Engine(*)\Utilization Percentage', 0, c.byref(counter)))
check(pdh.PdhCollectQueryData(query))
baseline = {pid: cpu(h) for pid, h, _ in items}
started = time.perf_counter()
time.sleep(float(sys.argv[2]) if len(sys.argv) > 2 else 8)
elapsed = time.perf_counter() - started
check(pdh.PdhCollectQueryData(query))
size, count = w.DWORD(), w.DWORD()
result = pdh.PdhGetFormattedCounterArrayW(counter, 0x200, c.byref(size), c.byref(count), None)
if result & 0xffffffff != 0x800007d2:
    check(result)
buffer = c.create_string_buffer(size.value)
check(pdh.PdhGetFormattedCounterArrayW(counter, 0x200, c.byref(size), c.byref(count), buffer))
gpu = c.cast(buffer, c.POINTER(Item))
for pid, h, path in items:
    one_core = (cpu(h) - baseline[pid]) / elapsed * 100
    engines = {gpu[i].name: gpu[i].formatted.value for i in range(count.value)
        if f'pid_{pid}_' in gpu[i].name and gpu[i].formatted.status in (0, 1)}
    print(json.dumps(dict(label=sys.argv[1], pid=pid, seconds=elapsed,
        cpu_one_core_percent=one_core, cpu_machine_percent=one_core / os.cpu_count(),
        gpu_engines=engines, path=path)))
    k.CloseHandle(h)
pdh.PdhCloseQuery(query)
