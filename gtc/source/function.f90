! magnetic field amplitude b=1/(1+r*cos(theta_0)), theta_0=theta+r*sin(theta)
function bfield(pdum,tdum)
  use global_parameters
  implicit none
  real(wp) :: bfield
  real(wp) :: pdum,tdum,r
  real(wp),external :: psi2r
  r=psi2r(pdum)
  bfield=1.0/(1.0+r*cos(tdum))
end function bfield

! transform from (psi_p, theta) to (x,z), d theta_0/d theta =1 + r cos(theta)
function boozer2x(pdum,tdum)
  use global_parameters
  implicit none
  real(wp) :: boozer2x
  real(wp) :: pdum,tdum,r
  real(wp),external :: psi2r
  r=psi2r(pdum)
  boozer2x=1.0+r*cos(tdum)
end function boozer2x

function boozer2z(pdum,tdum)
  use global_parameters
  implicit none
  real(wp) boozer2z
  real(wp) :: pdum,tdum,r
  real(wp),external :: psi2r
  r=psi2r(pdum)
  boozer2z=r*sin(tdum)
end function boozer2z

! transform from psi to r, and from r to psi
function psi2r(pdum)
  use global_parameters
  implicit none
  real(wp) :: psi2r
  real(wp) :: pdum
  psi2r=sqrt(max(1.0e-20_wp,2.0_wp*pdum))
end function psi2r

function r2psi(rdum)
  use global_parameters
  implicit none
  real(wp) :: r2psi
  real(wp) :: rdum
  r2psi=max(1.0e-20_wp,0.5_wp*rdum*rdum)
end function r2psi

