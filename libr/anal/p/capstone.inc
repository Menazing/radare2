#ifndef CSINC
#error Undefined CSINC
#endif

#ifndef CSINC_MODE
#define CSINC_MODE 0
#endif

#define TOKEN_PASTE(x, y) x ## y

#define CSINC_INS_ENDING CSINC_INS_ENDING_(CSINC)
#define CSINC_INS_ENDING_(y) TOKEN_PASTE(y, _INS_ENDING)

#define CSINC_ARCH CSINC_ARCH_(CSINC)
#define CSINC_ARCH_(y) TOKEN_PASTE(CS_ARCH_, y)

static R_TH_LOCAL int cs_obits = 32;
static R_TH_LOCAL int cs_omode = 0;
static R_TH_LOCAL csh cs_handle = 0;

static csh init_capstone (RAnal *a) {
	int mode = (CSINC_MODE);
	if (mode != cs_omode || a->config->bits != cs_obits) {
		if (cs_handle != 0) {
			cs_close (&cs_handle);
			cs_handle = 0; // unnecessary
		}
		cs_omode = mode;
		cs_obits = a->config->bits;
	}
	if (cs_handle == 0) {
		int ret = cs_open (CSINC_ARCH, mode, &cs_handle);
		if (ret != CS_ERR_OK) {
			R_LOG_ERROR ("Capstone failed: cs_open(%#x, %#x): %s", CSINC_ARCH, mode, cs_strerror (ret));
			cs_handle = 0;
		} else {
			cs_option (cs_handle, CS_OPT_DETAIL, CS_OPT_ON);
#if CS_API_MAJOR >= 4
			cs_option (cs_handle, CS_OPT_UNSIGNED, CS_OPT_ON);
#endif
		}
	}
	if (cs_handle) {
		if (a->config->syntax == R_ASM_SYNTAX_ATT) {
			cs_option (cs_handle, CS_OPT_SYNTAX, CS_OPT_SYNTAX_ATT);
#if CS_API_MAJOR >= 4
		} else if (a->config->syntax == R_ASM_SYNTAX_MASM) {
			cs_option (cs_handle, CS_OPT_SYNTAX, CS_OPT_SYNTAX_MASM);
#endif
		} else {
			cs_option (cs_handle, CS_OPT_SYNTAX, CS_OPT_SYNTAX_INTEL);
		}
	}

	return cs_handle;
}

static char *cs_mnemonics(RAnal *a, int id, bool json) {
	int i;
	csh cs_handle = init_capstone (a);
	if (cs_handle == 0) {
		return NULL;
	}

	PJ *pj = NULL;
	if (id != -1) {
		const char *name = cs_insn_name (cs_handle, id);
		if (name) {
			if (json) {
				pj = pj_new ();
				pj_a (pj);
				pj_s (pj, name);
				pj_end (pj);
				return pj_drain (pj);
			}
			return strdup (name);
		}
		return NULL;
	}

	RStrBuf *buf = NULL;
	if (json) {
		pj = pj_new ();
		pj_a (pj);
	} else {
		buf = r_strbuf_new ("");
	}
	for (i = 1; i < CSINC_INS_ENDING; i++) {
		const char *op = cs_insn_name (cs_handle, i);
		if (!op) {
			continue;
		}
		if (pj) {
			pj_s (pj, op);
		} else {
			r_strbuf_append (buf, op);
			r_strbuf_append (buf, "\n");
		}
	}
	if (pj) {
		pj_end (pj);
	}
	return pj? pj_drain (pj): r_strbuf_drain (buf);
}
