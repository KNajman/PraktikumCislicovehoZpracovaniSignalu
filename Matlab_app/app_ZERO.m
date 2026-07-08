function app=app_ZERO(x, Fs)
    arguments
        x (1,:) {mustBeNumeric} = 0;
        Fs (1,:) {mustBeNumeric} = 48000;
    end
    app.Fs=Fs;
    app.s=x;
end