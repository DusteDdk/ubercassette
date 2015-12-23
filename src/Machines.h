const struct MachineData MACHINE_VIC=
{
	"C64",
	1,
	VIC20,
	1108405,
	1022727,
	false,
	false,
	{0.75f, 0.75f},
	ConvertToCycles,
};

const struct MachineData MACHINE_C16=
{
	"C16",
	2,
	C16,
	886724,
	894886,
	false,
	false,
	{0.75f, 0.75f},
	ConvertToCycles,
};

const struct MachineData MACHINE_C64=
{
	"C64",
	0,
	C64,
	985248,
	1022727,
	false,
	false,
	{0.75f, 0.75f},
	ConvertToCycles,
};

const struct MachineData MACHINE_BBC=
{
	"BBC",
	0,
	BBC,
	2000000,
	2000000,
	false,
	true,
	{0.6f, 0.5f},
	ConvertToCycles,
};

const struct MachineData MACHINE_ELECTRON=
{
	"ELECTRON",
	0,
	ELECTRON,
	2000000,
	2000000,
	false,
	true,
	{0.6f, 0.5f},
	ConvertToCycles,
};

const struct MachineData MACHINE_SPECTRUM=
{
	"SPECTRUM",
	0,
	SPECTRUM,
	3500000,
	3500000,
	true,
	true,
	{0.5f, 0.5f},
	ConvertToCycles,
};

const struct MachineData MACHINE_AMSTRAD=
{
	"AMSTRAD",
	0,
	AMSTRAD,
	3500000,	/* TZX always uses 3.5MHz. */
	3500000,
	true,
	true,
	{0.5f, 0.5f},
	ConvertToCycles,
};
