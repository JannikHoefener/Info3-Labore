// wordpress plugin broke my inline JS so here it is
// used to be on http://eleccelerator.com/avr-timer-calculator/

function calc_total()
{
  var TotalTicks = document.getElementById("TotalTicks").value;

  var ClkFreq = document.getElementById("ClkFreq").value;
  var Prescaler = document.getElementById("Prescaler").value;
  var freq = ClkFreq / Prescaler;
  var TimerRes = document.getElementById("TimerRes").value;

  var TotalTicksOverflows = Math.floor(TotalTicks / Math.pow(2, TimerRes));
  var Remainder = TotalTicks - (TotalTicksOverflows * Math.pow(2, TimerRes));

  var RealTime = TotalTicks / freq;
  var NewFreq = freq / TotalTicks;

  update_all(TotalTicks, TotalTicksOverflows, Remainder, RealTime, NewFreq);
}

function calc_TotalTicksOverflowsRemainder()
{
  var TotalTicksOverflows = document.getElementById("Overflows").value;
  var Remainder = document.getElementById("Remainder").value;

  var ClkFreq = document.getElementById("ClkFreq").value;
  var Prescaler = document.getElementById("Prescaler").value;
  var freq = ClkFreq / Prescaler;
  var TimerRes = document.getElementById("TimerRes").value;

  var TotalTicks = TotalTicksOverflows * Math.pow(2, TimerRes) + parseInt(Remainder);

  var RealTime = TotalTicks / freq;
  var NewFreq = freq / TotalTicks;

  update_all(TotalTicks, TotalTicksOverflows, Remainder, RealTime, NewFreq);
}

function calc_RealTime()
{
  var RealTime = document.getElementById("RealTime").value;
  var NewFreq = 1 / RealTime;

  var ClkFreq = document.getElementById("ClkFreq").value;
  var Prescaler = document.getElementById("Prescaler").value;
  var freq = ClkFreq / Prescaler;
  var TimerRes = document.getElementById("TimerRes").value;

  var TotalTicks = RealTime * freq;

  var TotalTicksOverflows = Math.floor(TotalTicks / Math.pow(2, TimerRes));
  var Remainder = TotalTicks - (TotalTicksOverflows * Math.pow(2, TimerRes));

  update_all(TotalTicks, TotalTicksOverflows, Remainder, RealTime, NewFreq);
}

function calc_NewFreq()
{
  var NewFreq = document.getElementById("NewFreq").value;
  var RealTime = 1 / NewFreq;

  var ClkFreq = document.getElementById("ClkFreq").value;
  var Prescaler = document.getElementById("Prescaler").value;
  var freq = ClkFreq / Prescaler;
  var TimerRes = document.getElementById("TimerRes").value;

  var TotalTicks = RealTime * freq;

  var TotalTicksOverflows = Math.floor(TotalTicks / Math.pow(2, TimerRes));
  var Remainder = TotalTicks - (TotalTicksOverflows * Math.pow(2, TimerRes));

  update_all(TotalTicks, TotalTicksOverflows, Remainder, RealTime, NewFreq);
}
