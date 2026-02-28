#!/bin/sh

# Find spbm hwmon path
find_spbm_hwmon() {
    base_path="/sys/class/hwmon"
    for subdir in "$base_path"/*; do
        name_path="$subdir/name"
        if [ -f "$name_path" ]; then
            name=$(cat "$name_path")
            if [ "$name" = "spbm" ]; then
                echo "$subdir"
                return 0
            fi
        fi
    done
    return 1
}

# Read value and convert to Watts
read_val_w() {
    path="$1"
    if [ -f "$path" ]; then
        val=$(cat "$path" 2>/dev/null)
        case "$val" in
            ''|*[!0-9]*) echo "0.000" ;;
            *) awk "BEGIN { printf \"%.3f\", $val/1000000 }" ;;
        esac
    else
        echo "0.000"
    fi
}

# Help message
if [ "$1" = "--help" ]; then
    cat <<EOF
Usage: $0 [--csv] [program [args...]]

Options:
  --help       Show this help message
  --csv        Output in CSV format (default is table mode)

Behavior:
  If a program is specified, the script runs that program and monitors power
  until it exits. If no program is specified, the script continuously monitors
  power until manually stopped.

Metrics sampled each second:
  soc_pkg, sys_total, cpu_p, cpu_e, vcore, dc_input

Examples:
  $0
      Continuously output power data in table mode

  $0 --csv ./some_program arg1 arg2 > log.csv
      Run some_program and save power data in CSV format to log.csv
EOF
    exit 0
fi

hwmon_path=$(find_spbm_hwmon)
if [ -z "$hwmon_path" ]; then
    echo "Error: spbm driver not found in /sys/class/hwmon/"
    exit 1
fi

mode="table"
if [ "$1" = "--csv" ]; then
    mode="csv"
    shift
fi

print_table_header() {
    printf "%-8s | %12s | %12s | %12s | %12s | %12s | %12s\n" \
        "sec" "soc_pkg(W)" "sys_total(W)" "cpu_p(W)" "cpu_e(W)" "vcore(W)" "dc_input(W)"
    printf "%s\n" "-------------------------------------------------------------------------------------------------------------"
}

print_table_row() {
    sec="$1"; soc_pkg="$2"; sys_total="$3"; cpu_p="$4"; cpu_e="$5"; vcore="$6"; dc_input="$7"
    printf "%-8s | %12s | %12s | %12s | %12s | %12s | %12s\n" \
        "$sec" "$soc_pkg" "$sys_total" "$cpu_p" "$cpu_e" "$vcore" "$dc_input"
}

print_csv_header() {
    echo "sec,soc_pkg,sys_total,cpu_p,cpu_e,vcore,dc_input"
}

print_csv_row() {
    sec="$1"; soc_pkg="$2"; sys_total="$3"; cpu_p="$4"; cpu_e="$5"; vcore="$6"; dc_input="$7"
    echo "$sec,$soc_pkg,$sys_total,$cpu_p,$cpu_e,$vcore,$dc_input"
}

sample_loop() {
    pid="$1"
    sec=0
    while [ -z "$pid" ] || kill -0 "$pid" 2>/dev/null; do
        soc_pkg=$(read_val_w "$hwmon_path/power2_input")
        sys_total=$(read_val_w "$hwmon_path/power1_input")
        cpu_p=$(read_val_w "$hwmon_path/power4_input")
        cpu_e=$(read_val_w "$hwmon_path/power5_input")
        vcore=$(read_val_w "$hwmon_path/power6_input")
        dc_input=$(read_val_w "$hwmon_path/power8_input")

        if [ "$mode" = "csv" ]; then
            print_csv_row "$sec" "$soc_pkg" "$sys_total" "$cpu_p" "$cpu_e" "$vcore" "$dc_input"
        else
            print_table_row "$sec" "$soc_pkg" "$sys_total" "$cpu_p" "$cpu_e" "$vcore" "$dc_input"
        fi

        sec=$((sec + 1))
        sleep 1
    done
}

# Main logic
if [ "$mode" = "csv" ]; then
    print_csv_header
else
    print_table_header
fi

if [ $# -gt 0 ]; then
    "$@" &
    pid=$!
    sample_loop "$pid"
    wait "$pid"
else
    sample_loop ""
fi
