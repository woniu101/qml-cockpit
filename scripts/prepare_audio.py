"""Generate three original soft ambient demo tracks without network assets."""
from pathlib import Path
import math, wave, struct
from array import array
root=Path(__file__).resolve().parents[1]/'assets/ui/audio'
root.mkdir(parents=True,exist_ok=True)
rate=22050
for track,chord in enumerate([(130.8128,164.8138,195.9977),(110,130.8128,164.8138),(146.8324,174.6141,220)]):
    target=root/f'track{track}.wav'
    if target.exists(): continue
    samples=array('h')
    for i in range(rate*64):
        t=i/rate
        envelope=min(1,t/2,(64-t)/3)
        pulse=.7+.3*math.sin(t*.5)**2
        tone=sum(math.sin(2*math.pi*f*t+.08*math.sin(t*.3))*.10 for f in chord)
        melody=chord[(int(t/2)+track)%3]*2
        note=math.sin(2*math.pi*melody*t)*.055*math.exp(-(t%2)*2)
        samples.append(int(max(-1,min(1,(tone*pulse+note)*envelope))*24000))
    with wave.open(str(target),'wb') as f:
        f.setparams((1,2,rate,0,'NONE','not compressed'));f.writeframes(samples.tobytes())
    print(target.name)
