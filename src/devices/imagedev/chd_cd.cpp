// license:BSD-3-Clause
// copyright-holders:Nathan Woods, R. Belmont, Miodrag Milanovic
/*********************************************************************

    Code to interface the image code with CHD-CD core.

    Based on harddriv.c by Raphael Nabet 2003

*********************************************************************/

#include "emu.h"
#include "chd_cd.h"

#include "cdrom.h"
#include "romload.h"

// device type definition
DEFINE_DEVICE_TYPE(CDROM, cdrom_image_device, "cdrom_image", "CD-ROM Image")

//-------------------------------------------------
//  cdrom_image_device - constructor
//-------------------------------------------------

cdrom_image_device::cdrom_image_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock)
	: cdrom_image_device(mconfig, CDROM, tag, owner, clock)
{
}

cdrom_image_device::cdrom_image_device(const machine_config &mconfig, device_type type, const char *tag, device_t *owner, uint32_t clock)
	: device_t(mconfig, type,  tag, owner, clock)
	, device_image_interface(mconfig, *this)
	, m_cdrom_handle()
	, m_extension_list(nullptr)
	, m_interface(nullptr)
{
}
//-------------------------------------------------
//  cdrom_image_device - destructor
//-------------------------------------------------

cdrom_image_device::~cdrom_image_device()
{
}

//-------------------------------------------------
//  device_config_complete - perform any
//  operations now that the configuration is
//  complete
//-------------------------------------------------

void cdrom_image_device::device_config_complete()
{
	m_extension_list = "chd,cue,toc,nrg,gdi,iso,cdr";

	add_format("chdcd", "CD-ROM drive", m_extension_list, "");
}

//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void cdrom_image_device::device_start()
{
	// try to locate the CHD from a DISK_REGION
	chd_file *chd = machine().rom_load().get_disk_handle(owner()->tag());
	if (chd)
		m_cdrom_handle.reset(new cdrom_file(chd));
	else
		m_cdrom_handle.reset();
}

void cdrom_image_device::device_stop()
{
	m_cdrom_handle.reset();
	if (m_self_chd.opened())
		m_self_chd.close();
}

std::pair<std::error_condition, std::string> cdrom_image_device::call_load()
{
	std::error_condition err;
	chd_file *chd = nullptr;

	m_cdrom_handle.reset();

	if (!loaded_through_softlist()) {
		if (is_filetype("chd") && is_loaded()) {
			util::core_file::ptr proxy;
			err = util::core_file::open_proxy(image_core_file(), proxy);
			if (!err)
				err = m_self_chd.open(std::move(proxy)); // CDs are never writeable
			if (err)
				goto error;
			chd = &m_self_chd;
		}
	} else {
		chd = device().machine().rom_load().get_disk_handle(device().subtag("cdrom").c_str());
	}

	// open the CHD file
	if (chd)
		m_cdrom_handle.reset(new cdrom_file(chd));
	else
		m_cdrom_handle.reset(new cdrom_file(filename()));

	if (!m_cdrom_handle)
		goto error;

	return std::make_pair(std::error_condition(), std::string());

error:
	if (chd && chd == &m_self_chd)
		m_self_chd.close();
	return std::make_pair(err ? err : image_error::UNSPECIFIED, std::string());
}

void cdrom_image_device::call_unload()
{
	assert(m_cdrom_handle);
	m_cdrom_handle.reset();
	if (m_self_chd.opened())
		m_self_chd.close();
}
