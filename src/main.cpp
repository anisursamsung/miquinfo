#include "sys_info.hpp"
#include <miqutoolkit/miqutoolkit.hpp>
#include <xkbcommon/xkbcommon-keysyms.h>
#include <iostream>
#include <filesystem>
#include <cmath>
#include <vector>
#include <array>

using namespace miqu;
using namespace miquinfo;

static const std::array<std::string, 5> SECTION_SUBTITLES = {
    "System Health & At-a-Glance Metrics",
    "Processor, Motherboard, GPU & Displays",
    "Thermals, Cooling, Battery Health & AC",
    "Partitions, Filesystems & Memory Topology",
    "Wayland, Themes, Packages & Connectivity"
};

static void copy_to_clipboard(const std::string& text) {
    FILE* pipe = popen("wl-copy 2>/dev/null", "w");
    if (pipe) {
        fputs(text.c_str(), pipe);
        pclose(pipe);
    }
}

static std::shared_ptr<LinearLayout> create_spec_row(const std::string& label, const std::string& value) {
    auto row = std::make_shared<LinearLayout>(Orientation::Horizontal);
    row->set_layout_params(LayoutParams(static_cast<int>(LayoutDimension::MatchParent), static_cast<int>(LayoutDimension::WrapContent)));
    row->set_margin(0, 3, 0, 3);

    auto lbl = TextViewBuilder::create()
        ->text(label)
        ->muted()
        ->bold(true)
        ->build();
    lbl->set_layout_params(LayoutParams(static_cast<int>(LayoutDimension::WrapContent), static_cast<int>(LayoutDimension::WrapContent)));
    lbl->set_margin(0, 0, 16, 0);

    auto val = TextViewBuilder::create()
        ->text(value)
        ->textAlignment(TextAlignment::Right)
        ->ellipsize(true)
        ->build();
    val->set_layout_params(LayoutParams(0, static_cast<int>(LayoutDimension::WrapContent), 1.0f));

    row->add_view(lbl);
    row->add_view(val);
    return row;
}

static std::shared_ptr<LinearLayout> create_circular_gauge_item(
    const std::string& title,
    float fraction,
    const std::string& percent_str,
    const std::string& detail_str,
    bool invert_threshold = false)
{
    auto item = std::make_shared<LinearLayout>(Orientation::Horizontal);
    LayoutParams item_params(0, static_cast<int>(LayoutDimension::WrapContent), 1.0f);
    item_params.gravity = Gravity::CenterVertical;
    item->set_layout_params(item_params);
    item->set_margin(0, 4, 8, 4);

    Color prog_col;
    if (invert_threshold) {
        // Higher is better (e.g. Battery health)
        if (fraction >= 0.80f) {
            prog_col = Color(0.18f, 0.80f, 0.44f, 1.0f); // Green
        } else if (fraction >= 0.60f) {
            prog_col = Color(0.96f, 0.62f, 0.04f, 1.0f); // Amber
        } else {
            prog_col = Color(0.93f, 0.27f, 0.27f, 1.0f); // Red
        }
    } else {
        // Lower is better (e.g. Memory, Storage, CPU Load)
        if (fraction >= 0.90f) {
            prog_col = Color(0.93f, 0.27f, 0.27f, 1.0f); // Red
        } else if (fraction >= 0.75f) {
            prog_col = Color(0.96f, 0.62f, 0.04f, 1.0f); // Amber
        } else {
            prog_col = Color(0.12f, 0.65f, 0.95f, 1.0f); // Blue / Primary
        }
    }

    auto gauge = ProgressBarBuilder::create()
        ->circular(true)
        ->strokeWidth(6)
        ->progress(fraction)
        ->progressColor(prog_col)
        ->build();
    gauge->set_layout_params(LayoutParams(48, 48, Gravity::CenterVertical));
    gauge->set_margin(0, 0, 10, 0);

    auto text_col = std::make_shared<LinearLayout>(Orientation::Vertical);
    LayoutParams text_params(0, static_cast<int>(LayoutDimension::WrapContent), 1.0f);
    text_params.gravity = Gravity::CenterVertical;
    text_col->set_layout_params(text_params);

    auto title_lbl = TextViewBuilder::create()
        ->text(title)
        ->bold(true)
        ->build();
    title_lbl->set_margin(0, 0, 0, 1);

    auto percent_lbl = TextViewBuilder::create()
        ->text(percent_str)
        ->caption()
        ->bold(true)
        ->build();
    percent_lbl->set_margin(0, 0, 0, 1);

    auto detail_lbl = TextViewBuilder::create()
        ->text(detail_str)
        ->caption()
        ->muted()
        ->ellipsize(true)
        ->build();

    text_col->add_view(title_lbl);
    text_col->add_view(percent_lbl);
    text_col->add_view(detail_lbl);

    item->add_view(gauge);
    item->add_view(text_col);
    return item;
}

// =============================================================================
// PAGE POPULATORS
// =============================================================================

// 1. Overview Page
static void populate_overview(const std::shared_ptr<LinearLayout>& col, const SystemInfo& info) {
    col->clear_views();

    // Hero Brand Card
    auto hero_card = std::make_shared<CardView>();
    hero_card->set_padding(14, 12);
    hero_card->set_margin(0, 0, 0, 10);
    hero_card->set_layout_params(LayoutParams(static_cast<int>(LayoutDimension::MatchParent), static_cast<int>(LayoutDimension::WrapContent)));

    auto hero_row = std::make_shared<LinearLayout>(Orientation::Horizontal);
    hero_row->set_layout_params(LayoutParams(static_cast<int>(LayoutDimension::MatchParent), static_cast<int>(LayoutDimension::WrapContent), Gravity::CenterVertical));

    std::string miquland_icon = "miquland";
    if (ImageView::resolve_icon_path(miquland_icon).empty()) {
        miquland_icon = "/usr/share/icons/hicolor/256x256/apps/miquland.png";
        if (!std::filesystem::exists(miquland_icon)) {
            miquland_icon = "assets/miquland.png";
        }
    }

    auto logo = ImageViewBuilder::create()
        ->imageResource(miquland_icon)
        ->targetSize(68)
        ->build();
    logo->set_margin(0, 0, 14, 0);

    auto header_info = std::make_shared<LinearLayout>(Orientation::Vertical);
    header_info->set_layout_params(LayoutParams(0, static_cast<int>(LayoutDimension::WrapContent), 1.0f));

    auto brand_title = TextViewBuilder::create()
        ->text("Miquland")
        ->h1()
        ->bold(true)
        ->build();
    brand_title->set_margin(0, 0, 0, 2);
    header_info->add_view(brand_title);

    auto host_caption = TextViewBuilder::create()
        ->text(info.os_name + " • " + info.kernel)
        ->caption()
        ->muted()
        ->build();
    host_caption->set_margin(0, 0, 0, 6);
    header_info->add_view(host_caption);

    auto status_row = std::make_shared<LinearLayout>(Orientation::Horizontal);
    status_row->set_layout_params(LayoutParams(static_cast<int>(LayoutDimension::WrapContent), static_cast<int>(LayoutDimension::WrapContent), Gravity::CenterVertical));

    auto uptime_badge = TextViewBuilder::create()
        ->text("⏱ " + info.uptime)
        ->caption()
        ->bold(true)
        ->build();
    uptime_badge->set_margin(0, 0, 12, 0);
    status_row->add_view(uptime_badge);

    if (info.has_battery) {
        auto bat_badge = TextViewBuilder::create()
            ->text("🔋 " + info.battery_formatted)
            ->caption()
            ->bold(true)
            ->build();
        bat_badge->set_margin(0, 0, 8, 0);
        status_row->add_view(bat_badge);
    } else {
        auto ac_badge = TextViewBuilder::create()
            ->text("🔌 AC Online")
            ->caption()
            ->bold(true)
            ->build();
        status_row->add_view(ac_badge);
    }
    header_info->add_view(status_row);

    hero_row->add_view(logo);
    hero_row->add_view(header_info);
    hero_card->add_view(hero_row);
    col->add_view(hero_card);

    // 4-Gauge Metric Deck Card
    auto metric_card = std::make_shared<CardView>();
    metric_card->set_padding(14, 12);
    metric_card->set_margin(0, 0, 0, 10);
    metric_card->set_layout_params(LayoutParams(static_cast<int>(LayoutDimension::MatchParent), static_cast<int>(LayoutDimension::WrapContent)));

    auto metric_col = std::make_shared<LinearLayout>(Orientation::Vertical);
    metric_col->set_layout_params(LayoutParams(static_cast<int>(LayoutDimension::MatchParent), static_cast<int>(LayoutDimension::WrapContent)));

    auto metric_title = TextViewBuilder::create()->text("📊 Key System Gauges")->h2()->bold(true)->build();
    metric_title->set_margin(0, 0, 0, 4);
    metric_col->add_view(metric_title);
    metric_col->add_view(DividerViewBuilder::create()->margin(0, 6)->build());

    // Row 1: Memory & Storage
    auto gauges_row1 = std::make_shared<LinearLayout>(Orientation::Horizontal);
    gauges_row1->set_layout_params(LayoutParams(static_cast<int>(LayoutDimension::MatchParent), static_cast<int>(LayoutDimension::WrapContent)));
    gauges_row1->set_margin(0, 2, 0, 4);

    int ram_pct = static_cast<int>(info.ram_fraction * 100.0f);
    int disk_pct = static_cast<int>(info.disk_fraction * 100.0f);
    gauges_row1->add_view(create_circular_gauge_item("Memory", info.ram_fraction, std::to_string(ram_pct) + "% used", info.ram_formatted));
    gauges_row1->add_view(create_circular_gauge_item("Storage (/)", info.disk_fraction, std::to_string(disk_pct) + "% used", info.disk_formatted));
    metric_col->add_view(gauges_row1);

    // Row 2: CPU Load & Battery Health / Power
    auto gauges_row2 = std::make_shared<LinearLayout>(Orientation::Horizontal);
    gauges_row2->set_layout_params(LayoutParams(static_cast<int>(LayoutDimension::MatchParent), static_cast<int>(LayoutDimension::WrapContent)));
    gauges_row2->set_margin(0, 4, 0, 2);

    int cpu_pct = static_cast<int>(info.cpu_load_fraction * 100.0f);
    std::stringstream ss_load;
    ss_load << std::fixed << std::setprecision(2) << info.load_1m << " (1m avg)";
    gauges_row2->add_view(create_circular_gauge_item("CPU Load", info.cpu_load_fraction, std::to_string(cpu_pct) + "% load", ss_load.str()));

    if (info.has_battery && info.battery_health_percent > 0) {
        float bat_health_frac = info.battery_health_percent / 100.0f;
        std::string health_detail = std::to_string(info.battery_charge_full_mah) + " / " + std::to_string(info.battery_charge_design_mah) + " mAh";
        gauges_row2->add_view(create_circular_gauge_item("Battery Health", bat_health_frac, std::to_string(info.battery_health_percent) + "% health", health_detail, true));
    } else {
        gauges_row2->add_view(create_circular_gauge_item("Power Delivery", 1.0f, "100% Online", info.power_profile + " profile", true));
    }
    metric_col->add_view(gauges_row2);

    metric_card->add_view(metric_col);
    col->add_view(metric_card);

    // Quick Hardware Summary Card
    auto hw_card = std::make_shared<CardView>();
    hw_card->set_padding(14, 12);
    hw_card->set_margin(0, 0, 0, 4);
    hw_card->set_layout_params(LayoutParams(static_cast<int>(LayoutDimension::MatchParent), static_cast<int>(LayoutDimension::WrapContent)));

    auto hw_col = std::make_shared<LinearLayout>(Orientation::Vertical);
    hw_col->set_layout_params(LayoutParams(static_cast<int>(LayoutDimension::MatchParent), static_cast<int>(LayoutDimension::WrapContent)));

    auto hw_title = TextViewBuilder::create()->text("⚡ Quick Specs")->h2()->bold(true)->build();
    hw_title->set_margin(0, 0, 0, 4);
    hw_col->add_view(hw_title);
    hw_col->add_view(DividerViewBuilder::create()->margin(0, 6)->build());

    hw_col->add_view(create_spec_row("Processor", info.cpu_model));
    hw_col->add_view(create_spec_row("Graphics", info.gpu_model));
    hw_col->add_view(create_spec_row("Display", info.displays));
    hw_col->add_view(create_spec_row("Network IP", info.ip_address + " (" + info.network_interface + ")"));
    hw_col->add_view(create_spec_row("Sound Server", info.audio_server));

    hw_card->add_view(hw_col);
    col->add_view(hw_card);
}

// 2. Hardware Page
static void populate_hardware(const std::shared_ptr<LinearLayout>& col, const SystemInfo& info) {
    col->clear_views();

    // CPU Card
    auto cpu_card = std::make_shared<CardView>();
    cpu_card->set_padding(14, 12);
    cpu_card->set_margin(0, 0, 0, 10);
    cpu_card->set_layout_params(LayoutParams(static_cast<int>(LayoutDimension::MatchParent), static_cast<int>(LayoutDimension::WrapContent)));
    auto cpu_col = std::make_shared<LinearLayout>(Orientation::Vertical);
    cpu_col->set_layout_params(LayoutParams(static_cast<int>(LayoutDimension::MatchParent), static_cast<int>(LayoutDimension::WrapContent)));
    auto cpu_title = TextViewBuilder::create()->text("🖥️ Processor Architecture")->h2()->bold(true)->build();
    cpu_title->set_margin(0, 0, 0, 4);
    cpu_col->add_view(cpu_title);
    cpu_col->add_view(DividerViewBuilder::create()->margin(0, 6)->build());
    cpu_col->add_view(create_spec_row("Model", info.cpu_model));
    cpu_col->add_view(create_spec_row("Topology", std::to_string(info.cpu_cores) + " Physical Cores / " + std::to_string(info.cpu_threads) + " Threads"));
    if (info.cpu_freq_cur_mhz > 0) {
        cpu_col->add_view(create_spec_row("Current Clock", std::to_string(info.cpu_freq_cur_mhz) + " MHz (Max " + std::to_string(info.cpu_freq_max_mhz) + " MHz)"));
    }
    cpu_col->add_view(create_spec_row("Governor", info.cpu_governor));
    if (!info.cpu_cache_l3.empty()) {
        cpu_col->add_view(create_spec_row("Caches", "L1: " + info.cpu_cache_l1 + " • L2: " + info.cpu_cache_l2 + " • L3: " + info.cpu_cache_l3));
    }
    cpu_card->add_view(cpu_col);
    col->add_view(cpu_card);

    // Motherboard & BIOS Card
    auto mb_card = std::make_shared<CardView>();
    mb_card->set_padding(14, 12);
    mb_card->set_margin(0, 0, 0, 10);
    mb_card->set_layout_params(LayoutParams(static_cast<int>(LayoutDimension::MatchParent), static_cast<int>(LayoutDimension::WrapContent)));
    auto mb_col = std::make_shared<LinearLayout>(Orientation::Vertical);
    mb_col->set_layout_params(LayoutParams(static_cast<int>(LayoutDimension::MatchParent), static_cast<int>(LayoutDimension::WrapContent)));
    auto mb_title = TextViewBuilder::create()->text("🖳 Motherboard & Firmware")->h2()->bold(true)->build();
    mb_title->set_margin(0, 0, 0, 4);
    mb_col->add_view(mb_title);
    mb_col->add_view(DividerViewBuilder::create()->margin(0, 6)->build());
    mb_col->add_view(create_spec_row("Motherboard", info.mb_vendor + " " + info.mb_model));
    mb_col->add_view(create_spec_row("BIOS / UEFI", info.bios_vendor + " " + info.bios_version + " (" + info.bios_date + ")"));
    mb_col->add_view(create_spec_row("Boot Mode", info.boot_mode));
    mb_col->add_view(create_spec_row("Secure Boot", info.secure_boot ? "Enabled (Active)" : "Disabled"));
    mb_card->add_view(mb_col);
    col->add_view(mb_card);

    // Graphics & Displays Card
    auto gpu_card = std::make_shared<CardView>();
    gpu_card->set_padding(14, 12);
    gpu_card->set_margin(0, 0, 0, 4);
    gpu_card->set_layout_params(LayoutParams(static_cast<int>(LayoutDimension::MatchParent), static_cast<int>(LayoutDimension::WrapContent)));
    auto gpu_col = std::make_shared<LinearLayout>(Orientation::Vertical);
    gpu_col->set_layout_params(LayoutParams(static_cast<int>(LayoutDimension::MatchParent), static_cast<int>(LayoutDimension::WrapContent)));
    auto gpu_title = TextViewBuilder::create()->text("🎮 Graphics & Displays")->h2()->bold(true)->build();
    gpu_title->set_margin(0, 0, 0, 4);
    gpu_col->add_view(gpu_title);
    gpu_col->add_view(DividerViewBuilder::create()->margin(0, 6)->build());
    gpu_col->add_view(create_spec_row("Graphics Adapter", info.gpu_model));
    gpu_col->add_view(create_spec_row("Kernel Driver", info.gpu_driver));
    gpu_col->add_view(create_spec_row("Mesa / OpenGL", info.mesa_version));
    gpu_col->add_view(create_spec_row("Connected Displays", info.displays));
    gpu_card->add_view(gpu_col);
    col->add_view(gpu_card);
}

// 3. Sensors & Power Page
static void populate_sensors(const std::shared_ptr<LinearLayout>& col, const SystemInfo& info) {
    col->clear_views();

    // Thermals Card
    auto th_card = std::make_shared<CardView>();
    th_card->set_padding(14, 12);
    th_card->set_margin(0, 0, 0, 10);
    th_card->set_layout_params(LayoutParams(static_cast<int>(LayoutDimension::MatchParent), static_cast<int>(LayoutDimension::WrapContent)));
    auto th_col = std::make_shared<LinearLayout>(Orientation::Vertical);
    th_col->set_layout_params(LayoutParams(static_cast<int>(LayoutDimension::MatchParent), static_cast<int>(LayoutDimension::WrapContent)));
    auto th_title = TextViewBuilder::create()->text("🌡️ Thermal Sensors")->h2()->bold(true)->build();
    th_title->set_margin(0, 0, 0, 4);
    th_col->add_view(th_title);
    th_col->add_view(DividerViewBuilder::create()->margin(0, 6)->build());

    if (info.cpu_temp_c > 0) {
        std::string status = info.cpu_temp_c < 65 ? " (Normal)" : (info.cpu_temp_c < 85 ? " (Warm)" : " (Hot)");
        th_col->add_view(create_spec_row("CPU Temperature", std::to_string(info.cpu_temp_c) + "°C" + status));
    }
    if (info.pch_temp_c > 0) {
        th_col->add_view(create_spec_row("PCH Chipset", std::to_string(info.pch_temp_c) + "°C"));
    }
    if (info.wifi_temp_c > 0) {
        th_col->add_view(create_spec_row("Wireless Adapter", std::to_string(info.wifi_temp_c) + "°C"));
    }
    th_card->add_view(th_col);
    col->add_view(th_card);

    // Active Cooling Card
    auto fan_card = std::make_shared<CardView>();
    fan_card->set_padding(14, 12);
    fan_card->set_margin(0, 0, 0, 10);
    fan_card->set_layout_params(LayoutParams(static_cast<int>(LayoutDimension::MatchParent), static_cast<int>(LayoutDimension::WrapContent)));
    auto fan_col = std::make_shared<LinearLayout>(Orientation::Vertical);
    fan_col->set_layout_params(LayoutParams(static_cast<int>(LayoutDimension::MatchParent), static_cast<int>(LayoutDimension::WrapContent)));
    auto fan_title = TextViewBuilder::create()->text("🌀 Active Cooling & Fans")->h2()->bold(true)->build();
    fan_title->set_margin(0, 0, 0, 4);
    fan_col->add_view(fan_title);
    fan_col->add_view(DividerViewBuilder::create()->margin(0, 6)->build());
    fan_col->add_view(create_spec_row("Fan Speed", info.fan_rpm > 0 ? (std::to_string(info.fan_rpm) + " RPM") : "Passive / Zero RPM"));
    fan_card->add_view(fan_col);
    col->add_view(fan_card);

    // Battery Diagnostics Card
    if (info.has_battery) {
        auto bat_card = std::make_shared<CardView>();
        bat_card->set_padding(14, 12);
        bat_card->set_margin(0, 0, 0, 10);
        bat_card->set_layout_params(LayoutParams(static_cast<int>(LayoutDimension::MatchParent), static_cast<int>(LayoutDimension::WrapContent)));
        auto bat_col = std::make_shared<LinearLayout>(Orientation::Vertical);
        bat_col->set_layout_params(LayoutParams(static_cast<int>(LayoutDimension::MatchParent), static_cast<int>(LayoutDimension::WrapContent)));
        auto bat_title = TextViewBuilder::create()->text("🔋 Battery Health & Diagnostics")->h2()->bold(true)->build();
        bat_title->set_margin(0, 0, 0, 4);
        bat_col->add_view(bat_title);
        bat_col->add_view(DividerViewBuilder::create()->margin(0, 6)->build());

        bat_col->add_view(create_spec_row("Health", std::to_string(info.battery_health_percent) + "% of Design Capacity"));
        bat_col->add_view(create_spec_row("Full Charge Capacity", std::to_string(info.battery_charge_full_mah) + " mAh (Design: " + std::to_string(info.battery_charge_design_mah) + " mAh)"));
        bat_col->add_view(create_spec_row("Current Charge", std::to_string(info.battery_charge_now_mah) + " mAh (" + std::to_string(info.battery_percent) + "%)"));
        if (info.battery_power_watts > 0.05f) {
            std::stringstream ss_pwr;
            ss_pwr << std::fixed << std::setprecision(1) << info.battery_power_watts << " W (" << info.battery_voltage_v << " V)";
            bat_col->add_view(create_spec_row("Rate of Power", ss_pwr.str()));
        }
        bat_col->add_view(create_spec_row("Model & Chemistry", info.battery_technology + " • " + info.battery_model));
        if (info.battery_cycle_count > 0) {
            bat_col->add_view(create_spec_row("Cycle Count", std::to_string(info.battery_cycle_count)));
        }
        bat_card->add_view(bat_col);
        col->add_view(bat_card);
    }

    // Power Profile Card
    auto pwr_card = std::make_shared<CardView>();
    pwr_card->set_padding(14, 12);
    pwr_card->set_margin(0, 0, 0, 4);
    pwr_card->set_layout_params(LayoutParams(static_cast<int>(LayoutDimension::MatchParent), static_cast<int>(LayoutDimension::WrapContent)));
    auto pwr_col = std::make_shared<LinearLayout>(Orientation::Vertical);
    pwr_col->set_layout_params(LayoutParams(static_cast<int>(LayoutDimension::MatchParent), static_cast<int>(LayoutDimension::WrapContent)));
    auto pwr_title = TextViewBuilder::create()->text("⚡ Power Delivery & Profiles")->h2()->bold(true)->build();
    pwr_title->set_margin(0, 0, 0, 4);
    pwr_col->add_view(pwr_title);
    pwr_col->add_view(DividerViewBuilder::create()->margin(0, 6)->build());
    pwr_col->add_view(create_spec_row("Power Source", info.ac_online ? "AC Adapter Connected" : "Battery (Discharging)"));
    pwr_col->add_view(create_spec_row("Platform Profile", info.power_profile));
    pwr_card->add_view(pwr_col);
    col->add_view(pwr_card);
}

// 4. Storage & Memory Page
static void populate_storage(const std::shared_ptr<LinearLayout>& col, const SystemInfo& info) {
    col->clear_views();

    // Memory Breakdown Card
    auto mem_card = std::make_shared<CardView>();
    mem_card->set_padding(14, 12);
    mem_card->set_margin(0, 0, 0, 10);
    mem_card->set_layout_params(LayoutParams(static_cast<int>(LayoutDimension::MatchParent), static_cast<int>(LayoutDimension::WrapContent)));
    auto mem_col = std::make_shared<LinearLayout>(Orientation::Vertical);
    mem_col->set_layout_params(LayoutParams(static_cast<int>(LayoutDimension::MatchParent), static_cast<int>(LayoutDimension::WrapContent)));
    auto mem_title = TextViewBuilder::create()->text("🧠 Memory Topology")->h2()->bold(true)->build();
    mem_title->set_margin(0, 0, 0, 4);
    mem_col->add_view(mem_title);
    mem_col->add_view(DividerViewBuilder::create()->margin(0, 6)->build());

    mem_col->add_view(create_spec_row("Total Installed", std::to_string(info.ram_total_mb) + " MB"));
    mem_col->add_view(create_spec_row("In Active Use", std::to_string(info.ram_used_mb) + " MB (" + std::to_string(int(info.ram_fraction * 100)) + "%)"));
    mem_col->add_view(create_spec_row("Free Memory", std::to_string(info.ram_free_mb) + " MB"));
    mem_col->add_view(create_spec_row("Buffers & Cache", std::to_string(info.ram_buffers_mb + info.ram_cached_mb) + " MB"));
    mem_card->add_view(mem_col);
    col->add_view(mem_card);

    // Swap Card
    auto swap_card = std::make_shared<CardView>();
    swap_card->set_padding(14, 12);
    swap_card->set_margin(0, 0, 0, 10);
    swap_card->set_layout_params(LayoutParams(static_cast<int>(LayoutDimension::MatchParent), static_cast<int>(LayoutDimension::WrapContent)));
    auto swap_col = std::make_shared<LinearLayout>(Orientation::Vertical);
    swap_col->set_layout_params(LayoutParams(static_cast<int>(LayoutDimension::MatchParent), static_cast<int>(LayoutDimension::WrapContent)));
    auto swap_title = TextViewBuilder::create()->text("🔄 Swap Space & ZRAM")->h2()->bold(true)->build();
    swap_title->set_margin(0, 0, 0, 4);
    swap_col->add_view(swap_title);
    swap_col->add_view(DividerViewBuilder::create()->margin(0, 6)->build());
    swap_col->add_view(create_spec_row("Swap Allocated", info.swap_formatted));
    swap_col->add_view(create_spec_row("Swap Architecture", info.swap_is_zram ? "zram0 (Compressed RAM Block Device)" : "Disk Swap Partition"));
    swap_card->add_view(swap_col);
    col->add_view(swap_card);

    // Partitions Card
    auto part_card = std::make_shared<CardView>();
    part_card->set_padding(14, 12);
    part_card->set_margin(0, 0, 0, 4);
    part_card->set_layout_params(LayoutParams(static_cast<int>(LayoutDimension::MatchParent), static_cast<int>(LayoutDimension::WrapContent)));
    auto part_col = std::make_shared<LinearLayout>(Orientation::Vertical);
    part_col->set_layout_params(LayoutParams(static_cast<int>(LayoutDimension::MatchParent), static_cast<int>(LayoutDimension::WrapContent)));
    auto part_title = TextViewBuilder::create()->text("🗄️ Mounted Partitions & Filesystems")->h2()->bold(true)->build();
    part_title->set_margin(0, 0, 0, 4);
    part_col->add_view(part_title);
    part_col->add_view(DividerViewBuilder::create()->margin(0, 6)->build());

    for (size_t i = 0; i < info.partitions.size(); ++i) {
        const auto& p = info.partitions[i];
        if (i > 0) {
            part_col->add_view(DividerViewBuilder::create()->margin(0, 6)->build());
        }
        auto p_header = std::make_shared<LinearLayout>(Orientation::Horizontal);
        p_header->set_layout_params(LayoutParams(static_cast<int>(LayoutDimension::MatchParent), static_cast<int>(LayoutDimension::WrapContent)));
        p_header->set_margin(0, 2, 0, 2);

        auto p_mount = TextViewBuilder::create()->text(p.mount_point + " (" + p.fs_type + ")")->bold(true)->build();
        p_mount->set_layout_params(LayoutParams(static_cast<int>(LayoutDimension::WrapContent), static_cast<int>(LayoutDimension::WrapContent)));

        auto p_usage = TextViewBuilder::create()->text(p.formatted)->caption()->textAlignment(TextAlignment::Right)->build();
        p_usage->set_layout_params(LayoutParams(0, static_cast<int>(LayoutDimension::WrapContent), 1.0f));

        p_header->add_view(p_mount);
        p_header->add_view(p_usage);
        part_col->add_view(p_header);

        auto p_bar = ProgressBarBuilder::create()
            ->progress(p.fraction)
            ->trackHeight(6)
            ->build();
        p_bar->set_layout_params(LayoutParams(static_cast<int>(LayoutDimension::MatchParent), 6));
        p_bar->set_margin(0, 2, 0, 4);
        part_col->add_view(p_bar);
    }
    part_card->add_view(part_col);
    col->add_view(part_card);
}

// 5. System & Network Page
static void populate_system(const std::shared_ptr<LinearLayout>& col, const SystemInfo& info) {
    col->clear_views();

    // Desktop & Session Card
    auto de_card = std::make_shared<CardView>();
    de_card->set_padding(14, 12);
    de_card->set_margin(0, 0, 0, 10);
    de_card->set_layout_params(LayoutParams(static_cast<int>(LayoutDimension::MatchParent), static_cast<int>(LayoutDimension::WrapContent)));
    auto de_col = std::make_shared<LinearLayout>(Orientation::Vertical);
    de_col->set_layout_params(LayoutParams(static_cast<int>(LayoutDimension::MatchParent), static_cast<int>(LayoutDimension::WrapContent)));
    auto de_title = TextViewBuilder::create()->text("⚙️ Desktop & Wayland Session")->h2()->bold(true)->build();
    de_title->set_margin(0, 0, 0, 4);
    de_col->add_view(de_title);
    de_col->add_view(DividerViewBuilder::create()->margin(0, 6)->build());
    de_col->add_view(create_spec_row("OS", info.os_name));
    de_col->add_view(create_spec_row("Host", info.host_name));
    de_col->add_view(create_spec_row("Compositor", info.compositor));
    de_col->add_view(create_spec_row("Wayland Socket", info.wayland_display));
    de_col->add_view(create_spec_row("Toolkit", info.toolkit));
    de_col->add_view(create_spec_row("Shell & Terminal", info.shell + " (" + info.terminal + ")"));
    de_col->add_view(create_spec_row("Cursor Theme", info.cursor_theme));
    de_card->add_view(de_col);
    col->add_view(de_card);

    // Packages Card
    auto pkg_card = std::make_shared<CardView>();
    pkg_card->set_padding(14, 12);
    pkg_card->set_margin(0, 0, 0, 10);
    pkg_card->set_layout_params(LayoutParams(static_cast<int>(LayoutDimension::MatchParent), static_cast<int>(LayoutDimension::WrapContent)));
    auto pkg_col = std::make_shared<LinearLayout>(Orientation::Vertical);
    pkg_col->set_layout_params(LayoutParams(static_cast<int>(LayoutDimension::MatchParent), static_cast<int>(LayoutDimension::WrapContent)));
    auto pkg_title = TextViewBuilder::create()->text("📦 Package Repositories")->h2()->bold(true)->build();
    pkg_title->set_margin(0, 0, 0, 4);
    pkg_col->add_view(pkg_title);
    pkg_col->add_view(DividerViewBuilder::create()->margin(0, 6)->build());
    pkg_col->add_view(create_spec_row("Installed Packages", info.packages));
    pkg_card->add_view(pkg_col);
    col->add_view(pkg_card);

    // Network & Peripherals Card
    auto net_card = std::make_shared<CardView>();
    net_card->set_padding(14, 12);
    net_card->set_margin(0, 0, 0, 10);
    net_card->set_layout_params(LayoutParams(static_cast<int>(LayoutDimension::MatchParent), static_cast<int>(LayoutDimension::WrapContent)));
    auto net_col = std::make_shared<LinearLayout>(Orientation::Vertical);
    net_col->set_layout_params(LayoutParams(static_cast<int>(LayoutDimension::MatchParent), static_cast<int>(LayoutDimension::WrapContent)));
    auto net_title = TextViewBuilder::create()->text("🌐 Network & Peripherals")->h2()->bold(true)->build();
    net_title->set_margin(0, 0, 0, 4);
    net_col->add_view(net_title);
    net_col->add_view(DividerViewBuilder::create()->margin(0, 6)->build());
    net_col->add_view(create_spec_row("Interface", info.network_interface + " [" + info.network_type + "]"));
    net_col->add_view(create_spec_row("Local IP", info.ip_address));
    net_col->add_view(create_spec_row("Bluetooth", info.bluetooth_available ? "Controller Active (hci0)" : "Not Detected"));
    net_card->add_view(net_col);
    col->add_view(net_card);

    // Audio Card
    auto snd_card = std::make_shared<CardView>();
    snd_card->set_padding(14, 12);
    snd_card->set_margin(0, 0, 0, 4);
    snd_card->set_layout_params(LayoutParams(static_cast<int>(LayoutDimension::MatchParent), static_cast<int>(LayoutDimension::WrapContent)));
    auto snd_col = std::make_shared<LinearLayout>(Orientation::Vertical);
    snd_col->set_layout_params(LayoutParams(static_cast<int>(LayoutDimension::MatchParent), static_cast<int>(LayoutDimension::WrapContent)));
    auto snd_title = TextViewBuilder::create()->text("🔊 Audio Subsystem")->h2()->bold(true)->build();
    snd_title->set_margin(0, 0, 0, 4);
    snd_col->add_view(snd_title);
    snd_col->add_view(DividerViewBuilder::create()->margin(0, 6)->build());
    snd_col->add_view(create_spec_row("Sound Server", info.audio_server));
    snd_card->add_view(snd_col);
    col->add_view(snd_card);
}

// =============================================================================
// MAIN ENTRY POINT
// =============================================================================

int main(int argc, char** argv) {
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--print" || arg == "-p" || arg == "--summary" || arg == "-s") {
            SystemInfo info = SysInfoReader::gather();
            std::cout << info.to_plain_text();
            return 0;
        } else if (arg == "--markdown" || arg == "-m") {
            SystemInfo info = SysInfoReader::gather();
            std::cout << info.to_markdown();
            return 0;
        }
    }

    auto engine = AppEngine::create();
    if (!engine) {
        std::cerr << "[miquinfo] Failed to initialize AppEngine.\n";
        return 1;
    }

    SystemInfo info = SysInfoReader::gather();
    auto config = Config::get();
    std::shared_ptr<Window> window;

    // Root container (Zero outer padding so bottom nav docks flush to edges)
    auto root_container = std::make_shared<LinearLayout>(Orientation::Vertical);
    root_container->set_layout_params(LayoutParams(
        static_cast<int>(LayoutDimension::MatchParent),
        static_cast<int>(LayoutDimension::MatchParent)
    ));
    root_container->set_padding(0);

    // 5 Page Column Containers
    auto page0_col = std::make_shared<LinearLayout>(Orientation::Vertical);
    page0_col->set_layout_params(LayoutParams(static_cast<int>(LayoutDimension::MatchParent), static_cast<int>(LayoutDimension::WrapContent)));
    page0_col->set_padding(2, 2);

    auto page1_col = std::make_shared<LinearLayout>(Orientation::Vertical);
    page1_col->set_layout_params(LayoutParams(static_cast<int>(LayoutDimension::MatchParent), static_cast<int>(LayoutDimension::WrapContent)));
    page1_col->set_padding(2, 2);

    auto page2_col = std::make_shared<LinearLayout>(Orientation::Vertical);
    page2_col->set_layout_params(LayoutParams(static_cast<int>(LayoutDimension::MatchParent), static_cast<int>(LayoutDimension::WrapContent)));
    page2_col->set_padding(2, 2);

    auto page3_col = std::make_shared<LinearLayout>(Orientation::Vertical);
    page3_col->set_layout_params(LayoutParams(static_cast<int>(LayoutDimension::MatchParent), static_cast<int>(LayoutDimension::WrapContent)));
    page3_col->set_padding(2, 2);

    auto page4_col = std::make_shared<LinearLayout>(Orientation::Vertical);
    page4_col->set_layout_params(LayoutParams(static_cast<int>(LayoutDimension::MatchParent), static_cast<int>(LayoutDimension::WrapContent)));
    page4_col->set_padding(2, 2);

    // Initial Population
    populate_overview(page0_col, info);
    populate_hardware(page1_col, info);
    populate_sensors(page2_col, info);
    populate_storage(page3_col, info);
    populate_system(page4_col, info);

    std::shared_ptr<Toolbar> toolbar;
    std::shared_ptr<ViewPager> pager;
    std::shared_ptr<BottomNavigationView> bottom_nav;

    auto refresh_fn = [&]() {
        info = SysInfoReader::gather();
        populate_overview(page0_col, info);
        populate_hardware(page1_col, info);
        populate_sensors(page2_col, info);
        populate_storage(page3_col, info);
        populate_system(page4_col, info);
        if (window) window->schedule_redraw();
    };

    auto copy_fn = [&]() {
        copy_to_clipboard(info.to_markdown());
        if (toolbar) {
            toolbar->set_subtitle("📋 Report Copied to Clipboard!");
            if (window) window->schedule_redraw();
        }
    };

    // -------------------------------------------------------------------------
    // 1. TOP TOOLBAR
    // -------------------------------------------------------------------------
    toolbar = ToolbarBuilder::create()
        ->title("System Info")
        ->subtitle(SECTION_SUBTITLES[0])
        ->titleAlignment(TitleAlignment::Center)
        ->onRefresh(refresh_fn)
        ->onClose([engine]() {
            engine->quit();
        })
        ->build();
    toolbar->set_margin(14, 12, 14, 8);

    // Add Copy Action Button to Toolbar
    toolbar->add_action("📋", copy_fn);

    root_container->add_view(toolbar);

    // -------------------------------------------------------------------------
    // 2. VIEW PAGER (5 SECTIONS)
    // -------------------------------------------------------------------------
    pager = ViewPagerBuilder::create()
        ->currentPage(0)
        ->onPageChanged([&](int old_idx, int new_idx) {
            if (bottom_nav && bottom_nav->get_selected_index() != new_idx) {
                bottom_nav->set_selected_index(new_idx);
            }
            if (toolbar && new_idx >= 0 && new_idx < static_cast<int>(SECTION_SUBTITLES.size())) {
                toolbar->set_subtitle(SECTION_SUBTITLES[new_idx]);
            }
        })
        ->build();
    pager->set_layout_params(LayoutParams(
        static_cast<int>(LayoutDimension::MatchParent),
        0,
        1.0f
    ));
    pager->set_margin(14, 0, 14, 0);

    pager->add_page(ScrollViewBuilder::create()->contentView(page0_col)->build());
    pager->add_page(ScrollViewBuilder::create()->contentView(page1_col)->build());
    pager->add_page(ScrollViewBuilder::create()->contentView(page2_col)->build());
    pager->add_page(ScrollViewBuilder::create()->contentView(page3_col)->build());
    pager->add_page(ScrollViewBuilder::create()->contentView(page4_col)->build());

    root_container->add_view(pager);

    // -------------------------------------------------------------------------
    // 3. MASTER BOTTOM NAVIGATION BAR (5 SECTIONS - DOCKED FLUSH AT BOTTOM)
    // -------------------------------------------------------------------------
    bottom_nav = BottomNavigationViewBuilder::create()
        ->addItem("Overview", "ℹ️")
        ->addItem("Hardware", "🖥️")
        ->addItem("Sensors", "⚡")
        ->addItem("Storage", "💾")
        ->addItem("System", "🌐")
        ->selectedIndex(0)
        ->pillSize(44, 26)
        ->showDivider(true)
        ->barHeight(58)
        ->onItemSelected([&](int idx) {
            if (pager) pager->set_current_page(idx);
        })
        ->build();
    bottom_nav->set_layout_params(LayoutParams(
        static_cast<int>(LayoutDimension::MatchParent),
        58
    ));
    bottom_nav->set_margin(0, 0, 0, 0);
    root_container->add_view(bottom_nav);



    // -------------------------------------------------------------------------
    // 4. WAYLAND TOPLEVEL WINDOW
    // -------------------------------------------------------------------------
    window = WindowBuilder::create()
        ->title("System Information - miquinfo")
        ->appId("miquinfo")
        ->role(WindowRole::Toplevel)
        ->preferredSize(600, 680)
        ->closeOnEscape(true)
        ->contentView(root_container)
        ->onClose([engine]() {
            engine->quit();
        })
        ->onKey([&](const KeyPressEvent& ev) {
            if (!ev.pressed) return;
            if (ev.keysym == XKB_KEY_r || ev.keysym == XKB_KEY_R) {
                refresh_fn();
            } else if (ev.keysym == XKB_KEY_c || ev.keysym == XKB_KEY_C) {
                copy_fn();
            } else if (ev.keysym == XKB_KEY_1) {
                if (pager) pager->set_current_page(0);
            } else if (ev.keysym == XKB_KEY_2) {
                if (pager) pager->set_current_page(1);
            } else if (ev.keysym == XKB_KEY_3) {
                if (pager) pager->set_current_page(2);
            } else if (ev.keysym == XKB_KEY_4) {
                if (pager) pager->set_current_page(3);
            } else if (ev.keysym == XKB_KEY_5) {
                if (pager) pager->set_current_page(4);
            } else if (ev.keysym == XKB_KEY_q || ev.keysym == XKB_KEY_Q || ev.keysym == XKB_KEY_Escape) {
                engine->quit();
            }
        })
        ->build();

    if (!window) {
        std::cerr << "[miquinfo] Failed to initialize Wayland window.\n";
        return 1;
    }

    window->show();
    return engine->enter_loop();
}
