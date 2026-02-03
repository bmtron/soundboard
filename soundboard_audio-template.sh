#!/bin/bash

SINK_NAME="soundboard_sink"
MIC_SOURCE="your-input-device-name-here"

load() {
    echo "Loading soundboard audio setup..."
    
    # Create null sink
    echo "Creating null sink: $SINK_NAME"
    pactl load-module module-null-sink sink_name="$SINK_NAME" sink_properties=device.description="Soundboard"
    
    # Create loopback from mic to sink
    echo "Creating loopback from mic to sink..."
    pactl load-module module-loopback source="$MIC_SOURCE" sink="$SINK_NAME" latency_msec=1
    
    echo "Setup complete!"
    echo "In Discord, set your input device to: soundboard_sink.monitor"
}

unload() {
    echo "Unloading soundboard audio setup..."
    
    # Find and unload all loopback modules for this sink
    echo "Removing loopback modules..."
    pactl list modules short | grep "module-loopback.*$SINK_NAME" | awk '{print $1}' | while read -r module_id; do
        echo "  Unloading module $module_id"
        pactl unload-module "$module_id"
    done
    
    # Find and unload the null sink module
    echo "Removing null sink..."
    pactl list modules short | grep "module-null-sink.*$SINK_NAME" | awk '{print $1}' | while read -r module_id; do
        echo "  Unloading module $module_id"
        pactl unload-module "$module_id"
    done
    
    echo "Cleanup complete!"
}

status() {
    echo "=== Soundboard Audio Status ==="
    echo ""
    echo "Null sinks:"
    pactl list sinks short | grep "$SINK_NAME" || echo "  None found"
    echo ""
    echo "Monitor sources:"
    pactl list sources short | grep "$SINK_NAME" || echo "  None found"
    echo ""
    echo "Loopback modules:"
    pactl list modules short | grep "module-loopback.*$SINK_NAME" || echo "  None found"
    echo ""
}

case "$1" in
    load)
        load
        ;;
    unload)
        unload
        ;;
    reload)
        unload
        sleep 1
        load
        ;;
    status)
        status
        ;;
    *)
        echo "Usage: $0 {load|unload|reload|status}"
        echo ""
        echo "  load    - Create soundboard audio routing"
        echo "  unload  - Remove soundboard audio routing"
        echo "  reload  - Unload and reload (clean restart)"
        echo "  status  - Show current setup status"
        exit 1
        ;;
esac
