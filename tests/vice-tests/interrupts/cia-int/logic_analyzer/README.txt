[13:40]	_tlr: A quick question: So that half cycle shift is only present on those two last tests?
[13:41]	_tlr: If they are present on all tests but only affect the last two I'd understand it better.
[13:42]	skoe: no, it's only there in these two cases
[13:42]	skoe: ok, I didn't check all 5 * 24 cases, but many of them
[13:42]	skoe: weird,  I know.
[13:43]	skoe: I'll check some more cases on evening
[13:46]	_tlr: If you provide a raw dump later I can help some.
[13:47]	_tlr: My point was that the CPU could be sensitive to that half cycle shift only at that particular instruction cycle.
[13:47]	_tlr: I.e it always half shifts.
[13:48]	_tlr: Also I must assume that IRQ behaves the same, but that the edge trigger nature of the NMI input makes this anomaly happen differently.
[14:22]	skoe: You may want to download and install this software: http://www.zeroplus.com.tw/logic-analyzer_en/products.php?pdn=1&product_id=250
[14:22]	skoe: it runs on windows and possibly on wine (as you don't need USB)
[14:22]	skoe: With this you can load the original capture files
[15:15]	_pickle has joined
[16:10]	_tlr: Stable or beta?
[16:21]	vital_ is now known as vital
[16:55]	skoe: I guess stable is ok

-----

[21:03]	skoe: _tlr: http://skoe.de/irq/vice-nmi.txt
[21:09]	_tlr: Nice analysis!
[21:09]	_tlr: So which is the expected case?  the first or the second?
[21:11]	skoe: The first one is the usual one.
[21:11]	_tlr: That's was my guess too.
[21:11]	_tlr: It seems that the ack cycle in the second case prevents the edge from happening until one half cycle later.
[21:12]	skoe: In all other cases I checked I saw NMI going down during Phi2 = H.
[21:12]	skoe: This may be the reason, yes.
[21:12]	_tlr: I wasn't sure if the anomaly was a small pulse or like it looks here a delayed start just after the ack.
[21:12]	_tlr: Here it is clear that it is a delayed start of the edge just after a "dummy" ack.
[21:12]	skoe: I'm uploading the LA files, will still take a while.
[21:13]	_tlr: Great!
[21:13]	skoe: I wonder why it doesn't happen with the other addressing modes.
[21:13]	_tlr: I'm puzzled by your statement that this does not occur in the other instructions.
[21:13]	_tlr: midair. :)
[21:14]	_tlr: I assume it must have something to do with the bus timing from the cpu on that particular instruction cycle.
[21:14]	skoe: It was quite late yesterday, maybe I was wrong somewhere. It's nice that you can double-check it.
[21:14]	_tlr: Yes.
[21:14]	_tlr: If it happens always when "dummy" acking just before the edge then it is more understandable.
[21:16]	skoe: look at skoe.de/irq
[21:16]	skoe: I can tell you who to find a certain test case:
[21:16]	skoe: how
[21:17]	skoe: Start the program as demo and select a 32128
[21:17]	_tlr: btw, I got an idea that the 1541UII could be modified to include a bus logic analyzer that is triggerable from the c64.
[21:17]	skoe: Triggering was quite ease, you'll see soon:
[21:17]	_tlr: Gideon needs to provide a safe way to start prototype FPGA builds first.
[21:18]	skoe: can you open one of the files?
[21:18]	skoe: In the first line you see I/O2
[21:19]	skoe: I/O1
[21:19]	skoe: I wrote the test case number to $DEFF, that of 0 to 4 for the five addressing modes
[21:19]	skoe: Easy file contains one test case
[21:20]	_tlr: I got the files now, but haven't installed the application yet.
[21:20]	skoe: Are you going to do it? Then I'll go on with the explanation
[21:20]	_tlr: I will.
[21:20]	skoe: k
[21:21]	skoe: Where you see I/O1 = L and Databus = $NUMBER you know that this is the start of test case $NUMBER
[21:22]	skoe: Sometimes there are glitches. But you'll see where the pulse is about one Phi2 half-cycle long
[21:22]	_tlr: One question, can I avoid driver installs in the installer?
[21:22]	skoe: mhh, dunno.
[21:22]	_tlr: Will try then... 
[21:23]	skoe: The number of cycles was written to $DFFF. So when I/O2 pulses one cycle, you can see the number of cycles (the inner loop) on the data bus
[21:24]	skoe: I shortened the test from 0..24 (IIRC) to 0..15 cycles.
[21:24]	skoe: A7..A0 is the _data_ bus. The 'A' is for confusion only ;)
[21:26]	skoe: To get a quick impression about a certain test case, you can mark NMI on the left bar and then use F11/F12 to find the prev/next edge
[21:26]	_tlr: The installer was quite weird.  It seems I have the application now, but it said "installation failed" after installing.
[21:26]	skoe: win or wine?
[21:26]	_tlr: win xp.
[21:26]	skoe: mhh
[21:26]	_tlr: I did a custom install and removed some languages and the driver.
[21:27]	skoe: At least you have the app :)
[21:27]	skoe: Feel free to ask if there's something unclear
[21:28]	_tlr: Will try the dumps and look around.  Thanks.
[21:28]	skoe: btw: Seems the threshold voltage or the cables were not correct. Sometimes there seem to be glitches on the IRQ line and sometimes there are bits wrong on the data bus. But I'm quite sure that this is caused by the measurement.
[21:29]	_tlr: Ok.
[21:29]	_tlr: May I add these dumps to a subdirectory of cia-int?
[21:30]	_tlr: In that case, it would be useful to add the exact binary you ran too.
[21:30]	skoe: if you want
[21:30]	skoe: Oh!
[21:30]	skoe: You are right.
[21:30]	skoe: I found a NMI edge at a Phi1 cycle in 6526A-test0 too
[21:31]	skoe: The only reason why this did not a different pattern on the screen may be the instruction sequence of the test case.
[21:31]	skoe: But I'm sure that you'll find out the details :)
[21:32]	_tlr: Good.  I assume we will find those on the other test as well.   It will take me quite some time to go through this.
[21:32]	moiree has joined
[21:32]	_tlr: It's very useful that you did it for plain 6526 too.
[21:32]	skoe: =)
[21:33]	skoe: Yes it takes a while to follow the byte stream. It was a good help to compare it with the vice single step output
[21:33]	skoe: I'll put my source code on the server too.
[21:34]	_tlr: Good.
[21:34]	skoe: btw the txt file contained the original adresses while the LA captures contain the addresses of my modified program.
[21:34]	skoe: I mean what you see on the data bus on JSR, RTS etc.
[21:35]	_tlr: but the LA annotation in that file is modified to be correct?
[21:35]	skoe: yes
[21:36]	skoe: I faked it a little bit ;)
