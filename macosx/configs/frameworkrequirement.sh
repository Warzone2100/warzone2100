#!/bin/bash


cat > "${CODE_SIGN_REQUIREMENTS_PATH}" << EOF
designated => (
	(
		// Signed by dak180
		anchor = H"53defbcc2be297ccfd316c42b117e72d010a8151"
		and certificate leaf[subject.O] = dak180
	)
)
and identifier "net.wz2100.${PRODUCT_NAME}"

EOF
