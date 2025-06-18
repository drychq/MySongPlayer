// TimeUtils.js
.pragma library

/**
 * format timr
 * @param {number} milliseconds
 * @return {string} time string after format (MM:SS)
 */
function formatTime(milliseconds) {
    if (!milliseconds || milliseconds < 0) {
        return "00:00";
    }

    const totalSeconds = Math.floor(milliseconds / 1000);
    const minutes = Math.floor(totalSeconds / 60);
    const seconds = totalSeconds % 60;

    return String(minutes).padStart(2, '0') + ":" + String(seconds).padStart(2, '0');
}

/**
 * get slier precent
 * @param {number} position current position
 * @param {number} duration total time
 * @return {number} slider precent
 */
function getProgress(position, duration) {
    if (!duration || duration <= 0) {
        return 0;
    }
    return Math.max(0, Math.min(1, position / duration));
}

/**
 * compute position by slider side
 * @param {number} progress slier precent (0-1)
 * @param {number} duration total time
 * @return {number} position
 */
function getPositionFromProgress(progress, duration) {
    if (!duration || duration <= 0) {
        return 0;
    }
    return Math.max(0, Math.min(duration, progress * duration));
}


