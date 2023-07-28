import flite_module from './flite.mjs';
const flite = await flite_module();
const FS = flite.FS;

flite.ccall('init');

export default function speakToURL(message, voice="rms") {
        flite.ccall('speak', 'void', ['string', 'string', 'string'], [voice, message, 'test.wav']);
        const blob = new Blob([FS.readFile('test.wav')], { type: 'audio/wav' });
        const url = URL.createObjectURL(blob);
        return url;
}
