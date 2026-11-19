import re,sys
from pathlib import Path
t=Path(sys.argv[1]).read_text(encoding="utf-8",errors="replace")
assert re.search(r"CS6 PRESENT item=12 fp=1 tp=1 animated=0 rigid=1",t),"Weapon is still loaded as independent animated parts"
for stage in ["pistol","rifle","pistol-repeat"]:
 assert re.search(r"CS6 MODEL CHECK stage="+stage+r" .*?parts=1 stray=0 legacy=0 pass=1",t),"Missing intact single-mesh view: "+stage
for stage in ["standing","crouching","standing-restored"]:
 assert re.search(r"CS6 CAMERA CHECK stage="+stage+r" .*?pass=1",t),"Incorrect real camera eye height: "+stage
assert "Accessed None" not in t
print("PASS: whole single-mesh weapon, repeated swaps, no legacy overlap")
