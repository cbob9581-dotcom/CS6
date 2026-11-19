import sys,re
from pathlib import Path
t=Path(sys.argv[1]).read_text(encoding="utf-8",errors="replace")
rows=re.findall(r"CS6 PRESENT item=(\d+) fp=(\d+) tp=(\d+) animated=(\d+)",t)
assert rows,"No supplied gun model attached in runtime"
assert any(int(i)==12 and int(f)>5 and int(p)>5 and int(a)>0 for i,f,p,a in rows),"AK model and moving parts missing"
assert "Accessed None" not in t
print("PASS: supplied AK model attached for both views with animated parts")
