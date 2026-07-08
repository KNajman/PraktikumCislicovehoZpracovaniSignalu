function isOn=IsON_loop(isOnBegin)
    mlock
    persistent n
    if isempty(n)
        n = 0;
    end
    if isOnBegin==-1111
        n=0;
    else
        n = n+isOnBegin;
    end
    isOn=n;
end