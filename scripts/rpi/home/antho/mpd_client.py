import socket

MPD_HOST = "localhost"
MPD_PORT = 6600


def _mpd_escape(path):
    return path.replace("\\", "\\\\").replace('"', '\\"')


def mpd_command(command_line):
    """Sends one command to MPD's plain TCP protocol (localhost:6600); returns response lines
    (banner/trailing OK stripped). Raises on ACK error or connection failure."""
    sock = socket.create_connection((MPD_HOST, MPD_PORT), timeout=2)
    try:
        sock.recv(1024)  # banner: "OK MPD <version>\n"
        sock.sendall((command_line + "\n").encode("utf-8"))
        data = b""
        while not data.endswith(b"OK\n") and b"ACK " not in data:
            chunk = sock.recv(4096)
            if not chunk:
                break
            data += chunk
    finally:
        sock.close()

    text = data.decode("utf-8", errors="replace")
    if text.startswith("ACK "):
        raise RuntimeError(f"MPD error: {text.strip()}")
    lines = text.splitlines()
    if lines and lines[-1] == "OK":
        lines = lines[:-1]
    return lines


def mpd_lsinfo(path):
    """Returns an ordered [(is_directory, full_path), ...] for one MPD directory level."""
    lines = mpd_command(f'lsinfo "{_mpd_escape(path)}"')
    entries = []
    for line in lines:
        if line.startswith("directory: "):
            entries.append((True, line[len("directory: "):]))
        elif line.startswith("file: "):
            entries.append((False, line[len("file: "):]))
        elif line.startswith("playlist: "):
            entries.append((False, line[len("playlist: "):]))
        # Other keys (Last-Modified/Time/Artist/Title/...) describe the
        # most-recently-appended entry above and are not needed for browsing.
    return entries
