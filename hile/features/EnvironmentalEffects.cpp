/*
*	OXWARE developed by oxiKKK
*	Copyright (c) 2023
* 
*	This program is licensed under the MIT license. By downloading, copying, 
*	installing or using this software you agree to this license.
*
*	License Agreement
*
*	Permission is hereby granted, free of charge, to any person obtaining a 
*	copy of this software and associated documentation files (the "Software"), 
*	to deal in the Software without restriction, including without limitation 
*	the rights to use, copy, modify, merge, publish, distribute, sublicense, 
*	and/or sell copies of the Software, and to permit persons to whom the 
*	Software is furnished to do so, subject to the following conditions:
*
*	The above copyright notice and this permission notice shall be included 
*	in all copies or substantial portions of the Software. 
*
*	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS 
*	OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
*	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL 
*	THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
*	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
*	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS 
*	IN THE SOFTWARE.
*/

#include "precompiled.h"

Vector g_vecZero = Vector(0, 0, 0);

VarBoolean env_enable("env_enable", "Enables environmental effects", false);

VarBoolean env_rain("env_rain", "Enables raining", false);
VarInteger env_radius("env_radius", "Radius where to generate snow/rain/etc.", 300, 200, 1000);
VarBoolean env_ground_fog("env_ground_fog", "Enables ground fog", false);
VarInteger env_ground_fog_density("env_ground_fog_density", "Ground fog density", 1, 1, 14);

VarFloat env_rain_density("env_rain_density", "Controls density of the rain", 2.0f, 1.0f, 4.0f);
VarBoolean env_rain_ambient("env_rain_ambient", "Plays ambient raining sound", false);
VarBoolean env_rain_ambient_thunder("env_rain_ambient_thunder", "Plays ambient thunder sound", false);

VarBoolean env_snow("env_snow", "Enables snowing", false);
VarFloat env_snow_density("env_snow_density", "Controls density of the snow", 2.0f, 1.0f, 4.0f);
VarInteger env_snow_flake_size("env_snow_flake_size", "Snow flake size", 1, 1, 10);

void CEnvironmentalEffects::initialize()
{
	auto cl_enginefuncs = CMemoryHookMgr::the().cl_enginefuncs();

	m_weather_update_time = 0.0f;

	// sounds
	m_rain_sound_played = false;
	m_thunder_timer = cl_enginefuncs->pfnGetClientTime() + cl_enginefuncs->pfnRandomLong(60, 60 * 3);

	precache_sprites();

	// wind
	initialize_wind_variables();

	m_rain_sound_played = false;

	CConsole::the().info("In-Game environmental effects initialized");
	m_initialized = true;
}

void CEnvironmentalEffects::shutdown()
{
	// TODO: Without this the game crashes on unload however, also, after calling this, 
	//		 every other live particle will die (e.g. a smoke)
	auto iparticleman = CHLInterfaceHook::the().IParticleMan();
	if (iparticleman)
	{
		iparticleman->ResetParticles();
	}

	m_initialized = false;
}

void CEnvironmentalEffects::render()
{
	//
	// sound - has to run even if disabled (in order to disable sound when env_enable is off)
	//

	if (m_initialized)
	{
		play_ambient_rain_sound();
	}

	if (!env_enable.get_value())
	{
		return;
	}

	static bool as_lock = false;

	if (!m_initialized && !as_lock)
	{
		initialize();
	}

	// TODO: Figure out a better way of doing this, since this respawns all particles, it doesn't
	//		 just stop rendering them..
	if (CAntiScreen::the().hide_visuals() && !as_lock)
	{
		as_lock = true;
		shutdown();
		return;
	}
	else
	{
		as_lock = false;
	}

	auto cl_enginefuncs = CMemoryHookMgr::the().cl_enginefuncs();

	auto client_data = CLocalState::the().get_current_frame_clientdata();

	Vector weather_origin = CLocalState::the().get_origin();
	if (client_data->iuser1 > OBS_NONE && client_data->iuser1 != OBS_ROAMING)
	{
		hl::cl_entity_t* ent = cl_enginefuncs->pfnGetEntityByIndex(client_data->iuser2);
		if (ent)
		{
			weather_origin = ent->origin;
		}
	}

	weather_origin.z += 36.0f;
	m_weather_origin = weather_origin;

	update_wind_variables();

	//
	// rendering
	//
	
	// update only when it's time, to not flood the game
	if (m_weather_update_time <= cl_enginefuncs->pfnGetClientTime())
	{
		if (env_rain.get_value())
		{
			render_rain();
		}

		if (env_snow.get_value())
		{
			render_snow();
		}
	}

	m_old_time = cl_enginefuncs->pfnGetClientTime();
}

void CEnvironmentalEffects::precache_sprites()
{
	auto cl_enginefuncs = CMemoryHookMgr::the().cl_enginefuncs();

	m_snow_sprite = (hl::model_t*)cl_enginefuncs->pfnGetSpritePointer(cl_enginefuncs->pfnSPR_Load("sprites/effects/snowflake.spr"));
	m_rain_sprite = (hl::model_t*)cl_enginefuncs->pfnGetSpritePointer(cl_enginefuncs->pfnSPR_Load("sprites/effects/rain.spr"));
	m_ripple = (hl::model_t*)cl_enginefuncs->pfnGetSpritePointer(cl_enginefuncs->pfnSPR_Load("sprites/effects/ripple.spr"));
	m_water_splash = (hl::model_t*)cl_enginefuncs->pfnGetSpritePointer(cl_enginefuncs->pfnSPR_Load("sprites/wsplash3.spr"));
	m_wind_particle_sprite = (hl::model_t*)cl_enginefuncs->pfnGetSpritePointer(cl_enginefuncs->pfnSPR_Load("sprites/gas_puff_01.spr"));
}

void CEnvironmentalEffects::render_rain()
{
	Vector			angles, forward, right, up;
	hl::pmtrace_t	pmtrace;
	Vector			origin, vEndPos, vecWindOrigin;
	int				j, iWindParticle;
	float			flVel;
	Vector			vDir;
	float			f;
	char*			pszTexture;

	auto cl_enginefuncs = CMemoryHookMgr::the().cl_enginefuncs();

	m_weather_update_time = cl_enginefuncs->pfnGetClientTime() + (0.5 - ((float)env_rain_density.get_value() / 10.0f));

	Vector local_velocity = CLocalState::the().get_local_velocity_vec();
	Vector local_origin = CLocalState::the().get_origin();
	vDir = local_velocity;
	flVel = vDir.NormalizeInPlace();

	iWindParticle = 0;

	int density = env_rain_density.get_value() * 130;

	for (j = 0; j < density; j++)
	{
		origin = m_weather_origin;

		float radius = (float)env_radius.get_value() + 100.0f; // add a little bit more to rain
		origin.x += vDir.x + cl_enginefuncs->pfnRandomFloat(-radius, radius);
		origin.y += vDir.y + cl_enginefuncs->pfnRandomFloat(-radius, radius);
		origin.z += vDir.z + cl_enginefuncs->pfnRandomFloat(100.0f, 300.0f);

		f = local_velocity.x;
		vEndPos.x = origin.x + ((cl_enginefuncs->pfnRandomLong(0, 5) > 2) ? f : -f);
		vEndPos.y = origin.y + local_velocity.y;
		vEndPos.z = 8000.0f;

		cl_enginefuncs->pEventAPI->EV_SetTraceHull(2);
		cl_enginefuncs->pEventAPI->EV_PlayerTrace(origin, vEndPos, PM_WORLD_ONLY, -1, &pmtrace);

		pszTexture = (char*)cl_enginefuncs->pEventAPI->EV_TraceTexture(pmtrace.ent, origin, pmtrace.endpos);

		// create rain only from sky
		if (pszTexture && strncmp(pszTexture, "sky", 3) == 0)
		{
			create_raindrop(origin);

			if (env_ground_fog.get_value())
			{
				if (iWindParticle == (25 - env_ground_fog_density.get_value()))
				{
					iWindParticle = 1;

					vecWindOrigin.x = origin.x;
					vecWindOrigin.y = origin.y;
					vecWindOrigin.z = local_origin.z;

					if (cl_enginefuncs->pTriAPI->BoxInPVS(vecWindOrigin, vecWindOrigin))
					{
						vEndPos.z = 8000.0f;

						cl_enginefuncs->pEventAPI->EV_SetTraceHull(2);
						cl_enginefuncs->pEventAPI->EV_PlayerTrace(vecWindOrigin, vEndPos, PM_WORLD_ONLY, -1, &pmtrace);

						pszTexture = (char*)cl_enginefuncs->pEventAPI->EV_TraceTexture(pmtrace.ent, origin, pmtrace.endpos);

						// create rain only from sky
						if (pszTexture && strncmp(pszTexture, "sky", 3) == 0)
						{
							vEndPos.z = -8000.0f;

							cl_enginefuncs->pEventAPI->EV_SetTraceHull(2);
							cl_enginefuncs->pEventAPI->EV_PlayerTrace(vecWindOrigin, vEndPos, PM_WORLD_ONLY, -1, &pmtrace);

							create_wind_particle(pmtrace.endpos, 75.0f);
							iWindParticle = 1;
						}
					}
				}
				else
				{
					iWindParticle++;
				}
			}
		}
	}
}

void CEnvironmentalEffects::render_snow()
{
	hl::pmtrace_t	pmtrace;
	Vector			origin, vDir, vEndPos;
	float			flVel, f;
	int				j;
	char*			pszTexture;

	auto cl_enginefuncs = CMemoryHookMgr::the().cl_enginefuncs();

	m_weather_update_time = cl_enginefuncs->pfnGetClientTime() + (0.7 - ((float)env_rain_density.get_value() / 10.0f));

	Vector local_velocity = CLocalState::the().get_local_velocity_vec();
	Vector local_origin = CLocalState::the().get_origin();
	vDir = local_velocity;
	flVel = vDir.NormalizeInPlace();

	int density = env_snow_density.get_value() * 130;

	int iWindParticle = 0;
	Vector vecWindOrigin;
	for (j = 0; j < density; j++)
	{
		origin = m_weather_origin;

		float radius = (float)env_radius.get_value();
		origin.x += vDir.x + cl_enginefuncs->pfnRandomFloat(-radius, radius);
		origin.y += vDir.y + cl_enginefuncs->pfnRandomFloat(-radius, radius);
		origin.z += vDir.z + cl_enginefuncs->pfnRandomFloat(100.0f, 300.0f);

		f = local_velocity.x;
		vEndPos.x = origin.x + ((cl_enginefuncs->pfnRandomLong(0, 5) > 2) ? f : -f);
		vEndPos.y = origin.y + local_velocity.y;
		vEndPos.z = 8000.0f;

		cl_enginefuncs->pEventAPI->EV_SetTraceHull(2);
		cl_enginefuncs->pEventAPI->EV_PlayerTrace(origin, vEndPos, PM_WORLD_ONLY, -1, &pmtrace);

		pszTexture = (char*)cl_enginefuncs->pEventAPI->EV_TraceTexture(pmtrace.ent, origin, pmtrace.endpos);

		// create snow only from sky
		if (pszTexture && strncmp(pszTexture, "sky", 3) == 0)
		{
			create_snow_flake(origin);

			if (env_ground_fog.get_value())
			{
				if (iWindParticle == (25 - env_ground_fog_density.get_value() / 2))
				{
					iWindParticle = 1;

					vecWindOrigin.x = origin.x;
					vecWindOrigin.y = origin.y;
					vecWindOrigin.z = local_origin.z;

					if (cl_enginefuncs->pTriAPI->BoxInPVS(vecWindOrigin, vecWindOrigin))
					{
						vEndPos.z = 8000.0f;

						cl_enginefuncs->pEventAPI->EV_SetTraceHull(2);
						cl_enginefuncs->pEventAPI->EV_PlayerTrace(vecWindOrigin, vEndPos, PM_WORLD_ONLY, -1, &pmtrace);

						pszTexture = (char*)cl_enginefuncs->pEventAPI->EV_TraceTexture(pmtrace.ent, origin, pmtrace.endpos);

						// create rain only from sky
						if (pszTexture && strncmp(pszTexture, "sky", 3) == 0)
						{
							vEndPos.z = -8000.0f;

							cl_enginefuncs->pEventAPI->EV_SetTraceHull(2);
							cl_enginefuncs->pEventAPI->EV_PlayerTrace(vecWindOrigin, vEndPos, PM_WORLD_ONLY, -1, &pmtrace);

							create_wind_particle(pmtrace.endpos, 150.0f);
							iWindParticle = 1;
						}
					}
				}
				else
				{
					iWindParticle++;
				}
			}
		}
	}
}

void CEnvironmentalEffects::create_raindrop(const Vector& origin)
{
	if (!m_rain_sprite)
	{
		return;
	}

	auto cl_enginefuncs = CMemoryHookMgr::the().cl_enginefuncs();

	auto pParticle = new hl::CPartRainDrop();
	if (!pParticle)
	{
		return; // couldn't alloc more particles
	}

	pParticle->InitializeSprite(origin, g_vecZero, m_rain_sprite, 2.0f, 1.0f);

	strcpy(pParticle->m_szClassname, "particle_rain");

	pParticle->m_flStretchY = 40.0f;

	pParticle->m_vVelocity.x = m_wind_span.x * cl_enginefuncs->pfnRandomFloat(1.0f, 2.0f);
	pParticle->m_vVelocity.y = m_wind_span.y * cl_enginefuncs->pfnRandomFloat(1.0f, 2.0f);
	pParticle->m_vVelocity.z = cl_enginefuncs->pfnRandomFloat(-500.0f, -1800.0f);

	pParticle->m_iRendermode = hl::kRenderTransAlpha;
	pParticle->m_flGravity = 0;

	pParticle->m_vColor.x = pParticle->m_vColor.y = pParticle->m_vColor.z = 255.0f;

	pParticle->SetCollisionFlags(TRI_COLLIDEWORLD | TRI_COLLIDEKILL | TRI_WATERTRACE);
	pParticle->SetCullFlag((LIGHT_NONE | CULL_PVS | CULL_FRUSTUM_PLANE));

	pParticle->m_flDieTime = cl_enginefuncs->pfnGetClientTime() + 1.0f;

	m_particles.push_back(pParticle);
}

void CEnvironmentalEffects::create_snow_flake(const Vector& origin)
{
	if (!m_snow_sprite)
	{
		return;
	}

	auto cl_enginefuncs = CMemoryHookMgr::the().cl_enginefuncs();

	auto pParticle = new hl::CPartSnowFlake();
	if (!pParticle)
	{
		return; // couldn't alloc more particles
	}

	pParticle->InitializeSprite(origin, g_vecZero, m_snow_sprite, cl_enginefuncs->pfnRandomFloat(2.0, 2.5), 1.0);

	strcpy(pParticle->m_szClassname, "snow_particle");

	pParticle->m_iNumFrames = m_snow_sprite->numframes;

	pParticle->m_vVelocity.x = m_wind_span.x / cl_enginefuncs->pfnRandomFloat(1.0, 2.0);
	pParticle->m_vVelocity.y = m_wind_span.y / cl_enginefuncs->pfnRandomFloat(1.0, 2.0);
	pParticle->m_vVelocity.z = cl_enginefuncs->pfnRandomFloat(-100.0, -200.0);

	pParticle->SetCollisionFlags(TRI_COLLIDEWORLD);

	float r = cl_enginefuncs->pfnRandomFloat(0.0, 1.0);
	if (r < 0.1)
	{
		pParticle->m_vVelocity.x *= 0.5;
		pParticle->m_vVelocity.y *= 0.5;
	}
	else if (r < 0.2)
	{
		pParticle->m_vVelocity.z = -65.0;
	}
	else if (r < 0.3)
	{
		pParticle->m_vVelocity.z = -75.0;
	}

	pParticle->m_iRendermode = hl::kRenderTransAdd;

	pParticle->SetCullFlag(RENDER_FACEPLAYER | LIGHT_NONE | CULL_PVS | CULL_FRUSTUM_SPHERE);
	
	pParticle->m_flScaleSpeed = 0;
	pParticle->m_flDampingTime = 0;
	pParticle->m_iFrame = 0;
	pParticle->m_flMass = 1.0;
	pParticle->m_flSize = (int)env_snow_flake_size.get_value() + 1.0f;

	pParticle->m_flGravity = 0;
	pParticle->m_flBounceFactor = 0;

	pParticle->m_vColor.x = pParticle->m_vColor.y = pParticle->m_vColor.z = 128.0f;

	pParticle->m_flDieTime = cl_enginefuncs->pfnGetClientTime() + 3.0;

	pParticle->m_bSpiral = cl_enginefuncs->pfnRandomLong(0, 1) != 0;

	pParticle->m_flSpiralTime = cl_enginefuncs->pfnGetClientTime() + cl_enginefuncs->pfnRandomLong(2, 4);
}

void CEnvironmentalEffects::create_wind_particle(const Vector& origin, float max_size)
{
	if (!m_wind_particle_sprite)
	{
		return;
	}

	auto pParticle = new hl::CPartWind();
	if (!pParticle)
	{
		return; // couldn't alloc more particles
	}

	auto cl_enginefuncs = CMemoryHookMgr::the().cl_enginefuncs();

	Vector vOrigin;
	vOrigin = origin;
	vOrigin.z += 10.0;

	pParticle->InitializeSprite(vOrigin, g_vecZero, m_wind_particle_sprite, cl_enginefuncs->pfnRandomFloat(50.0, max_size), 1.0);

	pParticle->m_iNumFrames = m_wind_particle_sprite->numframes;

	strcpy(pParticle->m_szClassname, "wind_particle");

	pParticle->m_iFrame = cl_enginefuncs->pfnRandomLong(m_wind_particle_sprite->numframes / 2, m_wind_particle_sprite->numframes);

	pParticle->m_vVelocity.x = m_wind_span.x / cl_enginefuncs->pfnRandomFloat(1.0f, 2.0f);
	pParticle->m_vVelocity.y = m_wind_span.y / cl_enginefuncs->pfnRandomFloat(1.0f, 2.0f);

	if (cl_enginefuncs->pfnRandomFloat(0.0, 1.0) < 0.1)
	{
		pParticle->m_vVelocity.x *= 0.5;
		pParticle->m_vVelocity.y *= 0.5;
	}

	pParticle->m_iRendermode = hl::kRenderTransAlpha;

	pParticle->m_vAVelocity.z = cl_enginefuncs->pfnRandomFloat(-1.0, 1.0);

	pParticle->m_flScaleSpeed = 0.4;
	pParticle->m_flDampingTime = 0;

	pParticle->m_iFrame = 0;
	pParticle->m_flGravity = 0;

	pParticle->m_flMass = 1.0f;
	pParticle->m_flBounceFactor = 0;
	pParticle->m_vColor.x = pParticle->m_vColor.y = pParticle->m_vColor.z = cl_enginefuncs->pfnRandomFloat(100.0f, 140.0f);

	pParticle->m_flFadeSpeed = -1.0f;

	pParticle->SetCollisionFlags(TRI_COLLIDEWORLD);
	pParticle->SetCullFlag(RENDER_FACEPLAYER | LIGHT_NONE | CULL_PVS | CULL_FRUSTUM_SPHERE);

	pParticle->m_flDieTime = cl_enginefuncs->pfnGetClientTime() + cl_enginefuncs->pfnRandomFloat(5.0f, 8.0f);

	m_particles.push_back(pParticle);
}

void CEnvironmentalEffects::update_wind_variables()
{
	Vector			angles, forward, right, up;
	hl::pmtrace_t	pmtrace;
	Vector			origin;
	float			flVel, f;
	Vector			vDir;

	auto cl_enginefuncs = CMemoryHookMgr::the().cl_enginefuncs();

	// change direction of the rain if it's time
	if (m_next_wind_change_time <= cl_enginefuncs->pfnGetClientTime())
	{
		m_desired_wind_direction.x = cl_enginefuncs->pfnRandomFloat(-80.0f, 80.0f);
		m_desired_wind_direction.y = cl_enginefuncs->pfnRandomFloat(-80.0f, 80.0f);
		m_desired_wind_direction.z = 0;

		m_next_wind_change_time = cl_enginefuncs->pfnGetClientTime() + cl_enginefuncs->pfnRandomFloat(15.0f, 30.0f);

		m_desired_wind_speed = m_desired_wind_direction.NormalizeInPlace();

		m_ideal_wind_yaw = CMath::the().vec2yaw(m_desired_wind_direction);
	}

	vDir = m_wind_span;
	vDir.NormalizeInPlace();

	CMath::the().vector_angles(vDir, angles);

	f = CMath::the().angle_mod(angles[YAW]);
	if (m_ideal_wind_yaw != f)
	{
		flVel = (cl_enginefuncs->pfnGetClientTime() - m_old_time) * 5.0f;
		angles[YAW] = CMath::the().approach_angle(m_ideal_wind_yaw, f, flVel);
	}

	CMath::the().angle_vectors(angles, &forward, nullptr, nullptr);

	m_wind_span = forward * m_desired_wind_speed;
}

void CEnvironmentalEffects::initialize_wind_variables()
{
	auto cl_enginefuncs = CMemoryHookMgr::the().cl_enginefuncs();
	
	m_wind_span.x = cl_enginefuncs->pfnRandomFloat(-80.0f, 80.0f);
	m_wind_span.y = cl_enginefuncs->pfnRandomFloat(-80.0f, 80.0f);
	m_wind_span.z = 0;

	m_desired_wind_direction.x = cl_enginefuncs->pfnRandomFloat(-80.0f, 80.0f);
	m_desired_wind_direction.y = cl_enginefuncs->pfnRandomFloat(-80.0f, 80.0f);
	m_desired_wind_direction.z = 0;

	m_next_wind_change_time = cl_enginefuncs->pfnGetClientTime();
}

void CEnvironmentalEffects::play_ambient_rain_sound()
{
	bool env_enabled = env_enable.get_value();
	bool rain_enabled = env_rain.get_value();
	bool ambient_enabled = env_rain_ambient.get_value();

	if (ambient_enabled && env_enabled && rain_enabled && !m_rain_sound_played)
	{
		CEngineSoundPlayer::the().play_ambient_sound_unique(0, Vector(0, 0, 0), "sound/ambience/rain.wav", 1.0f, AMBIENT_EVERYWHERE, true);
		CConsole::the().info("played");
		m_rain_sound_played = true;
	}

	if ((!ambient_enabled || !env_enabled || !rain_enabled) && m_rain_sound_played)
	{
		CEngineSoundPlayer::the().stop_sound("sound/ambience/rain.wav");
		CConsole::the().info("stopped");
		m_rain_sound_played = false;
	}
	
	if (env_rain_ambient_thunder.get_value())
	{
		auto cl_enginefuncs = CMemoryHookMgr::the().cl_enginefuncs();

		float time = cl_enginefuncs->pfnGetClientTime();
		if (time > m_thunder_timer)
		{
			CEngineSoundPlayer::the().play_ambient_sound_unique(0, Vector(0, 0, 0), "sound/ambience/thunder_clap.wav", 1.0f, AMBIENT_EVERYWHERE, false);
			m_thunder_timer = time + cl_enginefuncs->pfnRandomLong(60, 60 * 3);
		}
	}
}

void CEnvironmentalEffects::check_particle_life()
{
}

//--------------------------------------------------------------------------------------------------
// Particle methods
//

namespace hl
{

void CPartRainDrop::Think(float time)
{
	Vector	right;
	Vector	vViewAngles;
	float	dotLength;

	auto cl_enginefuncs = CMemoryHookMgr::the().cl_enginefuncs();
	
	cl_enginefuncs->pfnGetViewAngles(vViewAngles);

	CMath::the().angle_vectors(vViewAngles, NULL, &right, NULL);

	m_vAngles[YAW] = vViewAngles[YAW];
	dotLength = DotProduct(m_vVelocity, right);
	m_vAngles[ROLL] = atan(dotLength / m_vVelocity.z) * (180.0 / M_PI);

	if (m_flBrightness < 155.0f)
	{
		m_flBrightness += 6.5f;
	}

	CBaseParticle::Think(time);
}

void CPartSnowFlake::Think(float time)
{
	float flFrametime;
	float fastFreq;

	auto cl_enginefuncs = CMemoryHookMgr::the().cl_enginefuncs();

	if (m_flBrightness < 130.0 && !m_bTouched)
	{
		m_flBrightness += 4.5;
	}

	Fade(time);
	Spin(time);

	if (m_flSpiralTime <= cl_enginefuncs->pfnGetClientTime())
	{
		m_bSpiral = !m_bSpiral;

		m_flSpiralTime = cl_enginefuncs->pfnGetClientTime() + cl_enginefuncs->pfnRandomLong(2, 4);
	}

	if (m_bSpiral && !m_bTouched)
	{
		flFrametime = time - CEnvironmentalEffects::the().get_old_time();
		fastFreq = sin(flFrametime * 5.0 + (float)(int)this); // TODO: Hmmm, this is weird

		m_vOrigin = m_vOrigin + m_vVelocity * flFrametime;
		m_vOrigin.x += (fastFreq * fastFreq) * 0.3f;
	}
	else
	{
		CalculateVelocity(time);
	}

	CheckCollision(time);
}

void CPartSnowFlake::Touch(Vector pos, Vector normal, int index)
{
	if (m_bTouched)
	{
		return;
	}

	m_bTouched = true;

	auto cl_enginefuncs = CMemoryHookMgr::the().cl_enginefuncs();

	SetRenderFlag(RENDER_FACEPLAYER);

	m_flOriginalBrightness = m_flBrightness;

	m_vVelocity = g_vecZero;

	m_iRendermode = kRenderTransAdd;

	m_flFadeSpeed = 0;
	m_flScaleSpeed = 0;
	m_flDampingTime = 0;
	m_iFrame = 0;
	m_flMass = 1.0;
	m_flGravity = 0;

	m_vColor.x = m_vColor.y = m_vColor.z = 128.0;

	m_flDieTime = cl_enginefuncs->pfnGetClientTime() + 0.5;

	m_flTimeCreated = cl_enginefuncs->pfnGetClientTime();
}

// TRUE if in water and returns vecResult.
// utility function
static hl::qboolean water_entry_point(hl::pmtrace_t* pTrace, const Vector& vecSrc, Vector& vecResult)
{
	Vector	result;
	int		endlevel, midlevel, startlevel;
	Vector	a, b, c;
	Vector	vecDir;
	float	fTolerance;

	auto cl_enginefuncs = CMemoryHookMgr::the().cl_enginefuncs();

	if (cl_enginefuncs->pfnPM_PointContents(pTrace->endpos, NULL) == cl_enginefuncs->pfnPM_PointContents((Vector&)vecSrc, NULL))
	{
		return FALSE;
	}

	a = vecSrc;
	b = pTrace->endpos;
	vecDir = b - a;

	fTolerance = 4.0f;
	while (vecDir.Length() > fTolerance)
	{
		vecDir /= 2.0f; // half the distance each iteration

		c = a + vecDir;

		if (cl_enginefuncs->pfnPM_PointContents(c, NULL) == cl_enginefuncs->pfnPM_PointContents(a, NULL))
		{
			a = c;
			result = c;
		}
		else
		{
			b = c;
			result = a;
		}

		vecDir = b - result;
	}

	vecResult = result;
	return TRUE;
}

void CPartRainDrop::Touch(Vector pos, Vector normal, int index)
{
	Vector			start, end, pt;
	pmtrace_t		tr;
	Vector			vRippleAngles, angles;
	CBaseParticle*	pParticle;
	model_t*		pSprite;

	if (m_bTouched)
	{
		return;
	}

	m_bTouched = true;

	auto cl_enginefuncs = CMemoryHookMgr::the().cl_enginefuncs();

	start = m_vOrigin;
	start.z += 32.0f;

	end = m_vOrigin;
	end.z -= 16.0f;

	cl_enginefuncs->pEventAPI->EV_PlayerTrace(start, end, PM_WORLD_ONLY, -1, &tr);

	angles.x = normal.x;
	angles.y = normal.y;
	angles.z = -normal.z;

	CMath::the().vector_angles(angles, vRippleAngles);

	//
	// see if we dropped into water
	//
	if (water_entry_point(&tr, start, pt))
	{
		pParticle = new CBaseParticle();
		if (!pParticle)
		{
			return; // couldn't alloc more particles
		}

		pSprite = CEnvironmentalEffects::the().get_ripple_sprite();

		pParticle->InitializeSprite(pt, vRippleAngles, pSprite, 15.0f, 110.0f);

		pParticle->m_iRendermode = kRenderTransAdd;
		pParticle->m_flScaleSpeed = 1.0f;
		pParticle->m_flFadeSpeed = 2.0f;

		pParticle->m_vColor.x = pParticle->m_vColor.y = pParticle->m_vColor.z = 255.0f;

		pParticle->SetCullFlag(LIGHT_INTENSITY | CULL_PVS | CULL_FRUSTUM_SPHERE);

		pParticle->m_flDieTime = cl_enginefuncs->pfnGetClientTime() + 2.0f;
	}
	else
	{
		pParticle = new CBaseParticle();
		if (!pParticle)
		{
			return; // couldn't alloc more particles
		}

		pSprite = CEnvironmentalEffects::the().get_splash_sprite();

		pParticle->InitializeSprite(m_vOrigin + normal, Vector(90.0f, 0, 0), pSprite, cl_enginefuncs->pfnRandomLong(20, 25), 125.0f);

		pParticle->m_iRendermode = kRenderTransAdd;

		pParticle->m_flMass = 1.0f;
		pParticle->m_flGravity = 0.1f;

		pParticle->m_vColor.x = pParticle->m_vColor.y = pParticle->m_vColor.z = 255.0f;

		pParticle->m_iNumFrames = pSprite->numframes - 1;
		pParticle->m_iFramerate = cl_enginefuncs->pfnRandomLong(30, 45);
		pParticle->SetCollisionFlags(TRI_ANIMATEDIE);
		pParticle->SetRenderFlag(RENDER_FACEPLAYER);
		pParticle->SetCullFlag(LIGHT_INTENSITY | CULL_PVS | CULL_FRUSTUM_SPHERE);

		pParticle->m_flDieTime = cl_enginefuncs->pfnGetClientTime() + 0.3f;
	}

	CEnvironmentalEffects::the().add_new_particle(pParticle);
}

void CPartWind::Think(float flTime)
{
	auto cl_enginefuncs = CMemoryHookMgr::the().cl_enginefuncs();

	if (m_flDieTime - flTime <= 3.0)
	{
		if (m_flBrightness > 0.0)
		{
			m_flBrightness -= (flTime - m_flTimeCreated) * 0.4;
		}

		if (m_flBrightness < 0.0)
		{
			m_flBrightness = 0;
			flTime = m_flDieTime = cl_enginefuncs->pfnGetClientTime();
		}
	}
	else
	{
		if (m_flBrightness < 105.0)
		{
			m_flBrightness += (flTime - m_flTimeCreated) * 5.0 + 4.0;
		}
	}

	CBaseParticle::Think(flTime);
}

} // namespace hl